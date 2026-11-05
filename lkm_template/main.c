// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "kernel.h"

#if !defined(__aarch64__)
static_assert(1 == 0, "ARM64 only!");
#endif

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "lkm_template: " fmt
#endif

static uintptr_t kallsyms_lookup_name_fn = 0x0;
static __nocfi unsigned long __kallsyms_lookup_name(const char *name)
{
	return ((typeof (__kallsyms_lookup_name) *)kallsyms_lookup_name_fn)(name);
}
#define kallsyms_lookup_name __kallsyms_lookup_name

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0) // probe_kernel_read
__weak long copy_from_kernel_nofault(void *dst, const void *src, size_t size)
{
	// https://elixir.bootlin.com/linux/v5.2.21/source/mm/maccess.c#L27
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);
	pagefault_disable();
	long ret = __copy_from_user_inatomic(dst, (__force const void __user *)src, size);
	pagefault_enable();
	set_fs(old_fs);

	return ret ? -EFAULT : 0;
}
#endif

static int bootstrap_kallsyms_lookup_name(uintptr_t ptr)
{
	extern long copy_from_kernel_nofault(void *dst, const void *src, size_t size);

	constexpr int max_iter = 400000;
	constexpr char sym[] = "kallsyms_lookup_name+";
	uintptr_t anchor = ptr;

	char buf[KSYM_SYMBOL_LEN];
	uint32_t ibuf;
	int iter = 0;

	uintptr_t curr = ptr;
	bool up = true;

	// record
	uintptr_t r_high = 0x0;
	uintptr_t r_low = 0x0;

	/**
	 * radiating scan, expect some cache trashing.
	 * however for this one it is worth it.
	 */
start_r:
	if (iter >= max_iter) {
		pr_info("%s: bootsrap fail! iter: %d range: 0x%lx ~ 0x%lx\n", __func__, iter, r_high, r_low);
		return -ERANGE;
	}

	if (!!copy_from_kernel_nofault(&ibuf, (void *)curr, sizeof(uint32_t)))
		goto inc;

	if (sprint_symbol(buf, curr) < sizeof(sym))
		goto inc;

	if (strstarts(buf, sym))
		goto found_candidate;

inc: // assumes that kallsyms_lookup_name is atleast 32 insns
	if (up) {
		iter++;
		r_high = curr;
		curr = anchor + sizeof(uint32_t) * 16 * iter;
		up = false;
	} else {
		r_low = curr;
		curr = anchor - sizeof(uint32_t) * 16 * iter;
		up = true;
	}

	goto start_r;

found_candidate:;
	char *sl = strchr(buf, '/');
	if (!sl)
		goto inc;
	
	sl[0] = 0;

	char *pl = strchr(buf, '+');
	if (!pl)
		goto inc;

	pr_info("candidate found: %s @ 0x%lx\n", buf, curr);

	unsigned long offset;
	if (kstrtoul(pl + 1, 0, &offset))
		goto inc;

	uintptr_t test_p = curr - offset;
	if (!(sprint_symbol(buf, test_p) > 0))
		goto inc;

	if (!strstr(buf, "+0x0"))
		goto inc;

	curr = curr - offset;
	pr_info("%s: %s found at 0x%lx iter: %d range: 0x%lx ~ 0x%lx\n", __func__, buf, curr, iter, r_high, r_low);
	*(volatile uintptr_t *)&kallsyms_lookup_name_fn = curr;
	smp_mb();
	return 0;
}

/**
 *  kallsyms_lookup_size_offset from sprint_symbol actually
 */
static int compat_kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize, unsigned long *offset)
{
	char buf[KSYM_SYMBOL_LEN];
	unsigned long u_sz;
	unsigned long u_off;

	if (!addr || !symbolsize || !offset)
		return 0;

	if (!(sprint_symbol(buf, addr) > 0))
		return 0;		
	
//	pr_info("symbol is: %s \n", buf);

	char *c_sz = strchr(buf, '/');
	if (!c_sz)
		return 0;		

	if (kstrtoul(c_sz + 1, 0, &u_sz))
		return 0;		

	// null '/', strtok called back.
	c_sz[0] = 0;

	char *c_off = strchr(buf, '+');
	if (!c_off)
		return 0;	

	if (kstrtoul(c_off + 1, 0, &u_off))
		return 0;		

//	pr_info("symbol_size: %zu \n", u_sz);
//	pr_info("symbol_offset: %zu \n", u_off);

	*symbolsize = u_sz;
	*offset = u_off;

	return 1;
}
#define kallsyms_lookup_size_offset compat_kallsyms_lookup_size_offset

static uintptr_t aarch64_insn_adrp_get_offset_fn = 0x0;
static __nocfi int32_t __aarch64_insn_adrp_get_offset(uint32_t insn)
{	
	return ((typeof(__aarch64_insn_adrp_get_offset) *)aarch64_insn_adrp_get_offset_fn)(insn);
}
#define aarch64_insn_adrp_get_offset __aarch64_insn_adrp_get_offset

static size_t get_ksym_size(uintptr_t symbol_addr, size_t retfail)
{
	unsigned long offset = 0;
	unsigned long symbolsize = 0;

	// means lookup failed, just return retfail
	if (!kallsyms_lookup_size_offset(symbol_addr, &symbolsize, &offset))
		return retfail;		

	if (!symbolsize && !offset)
		return retfail;

	return symbolsize;
}

/**
 * locate currently used sys_call_table.
 */
static int hunt_sct_adrp(uintptr_t insn_addr_start, ptrdiff_t scan_depth)
{
	uintptr_t insn_p = insn_addr_start;
	uintptr_t insn_end = insn_addr_start + scan_depth;
	uint32_t raw_insn = 0;

scan:
	if (insn_p >= insn_end)
		return -ERANGE;

	if (!!copy_from_kernel_nofault((void *)&raw_insn, (void *)insn_p, sizeof(uint32_t)))
		goto increment_adrp;

	// aarch64_insn_is_##abbr, arch/arm64/include/asm/insn.h
	if (!aarch64_insn_is_adrp(raw_insn))
		goto increment_adrp;

	pr_info("0x%lx insn is adrp! .inst 0x%lx \n", insn_p, raw_insn);

	uintptr_t pc_pg = insn_p & ~0xFFFUL; // target, aligned to 4096
	uintptr_t tgt = pc_pg + (long)aarch64_insn_adrp_get_offset(raw_insn);

	pr_info("tgt: 0x%lx \n", tgt);

	constexpr int max_tries = 4096 / 8;

	uintptr_t pg_start = tgt;
	uintptr_t curr = tgt;
	uintptr_t pbuf;
	uintptr_t dummy;
	int tries = 0;

scan_pg:
	if (tries > max_tries)
		goto increment_adrp;

	if (!!copy_from_kernel_nofault((void *)&pbuf, (void *)curr, sizeof(uintptr_t)))
		goto increment_pg_p;

	if (!!copy_from_kernel_nofault((void *)&dummy, (void *)pbuf, sizeof(uintptr_t)))
		goto increment_pg_p;

	char buf[KSYM_SYMBOL_LEN];

	if (!(sprint_symbol(buf, pbuf) > 0))
		goto increment_pg_p;

	if (!strstr(buf, "sys_io_setup"))
		goto increment_pg_p;

	pr_info("symbol: %s @ 0x%lx &sys_call_table[0]: 0x%lx\n", buf, pbuf, curr);
	return 0;

increment_pg_p:
	tries++;
	curr = curr + sizeof(uintptr_t);
	goto scan_pg;

increment_adrp:
	insn_p = insn_p + sizeof(uint32_t);
	goto scan;

}

static int resolve_svc_handler(const char *native_sym, const char *compat_sym, ptrdiff_t scan_depth)
{
	uintptr_t target = kallsyms_lookup_name(native_sym);
	if (!target)
		return -ENOENT;

	ssize_t new_scan_depth = get_ksym_size(target, scan_depth);
	pr_info("target: %s @ 0x%lx sz: %zu\n", native_sym, target, new_scan_depth);

	int ret = hunt_sct_adrp(target, new_scan_depth);
	if (ret)
		return ret;

	return 0;
}

static void hunt_sys_call_table(void)
{
	aarch64_insn_adrp_get_offset_fn = (uintptr_t)kallsyms_lookup_name("aarch64_insn_adrp_get_offset");
	if (!aarch64_insn_adrp_get_offset_fn)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)
	int ret = resolve_svc_handler("do_el0_svc", "do_el0_svc_compat", 64 * 4);
	if (!ret)
		return;

	ret = resolve_svc_handler("el0_svc_handler", "el0_svc_compat_handler", 64 * 4);
	if (!ret)
		return;

	// for heavily inlined shit
	resolve_svc_handler("el0_svc", "el0_svc_compat", 256 * 4);
#else
	resolve_svc_handler("el0_svc", "el0_svc_compat", 16 * 4);
#endif
}

static int __init lkm_template_init(void) 
{
	pr_info("LKM: Hello World!\n");

	constexpr char sym[] = "sprint_symbol";
	uintptr_t addr = (uintptr_t)&sprint_symbol;
	pr_info("%s: %s @ 0x%lx \n", __func__, sym, addr);

	int ret = bootstrap_kallsyms_lookup_name(addr);
	if (ret)
		return ret;

	uintptr_t verify = kallsyms_lookup_name(sym);
	pr_info("verify result: 0x%lx \n", verify);

	unsigned long sz = 0;
	unsigned long off = 0;

	ret = compat_kallsyms_lookup_size_offset(verify, &sz, &off);
	pr_info("compat_kallsyms_lookup_size_offset: ret: %d size: %lu offset: %lu \n", ret, sz, off);

	hunt_sys_call_table();

	return -EAGAIN;
}

static void __exit lkm_template_exit(void)
{
	pr_info("LKM: Goodbye\n");
}

module_init(lkm_template_init);
module_exit(lkm_template_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xx");
MODULE_DESCRIPTION("lkm hello");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver");
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
