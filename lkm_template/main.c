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

	constexpr int max_iter = 100000;
	constexpr char sym[] = "kallsyms_lookup_name+0x0/";
	uintptr_t anchor = ptr;

	char buf[KSYM_SYMBOL_LEN];
	uint32_t ibuf;
	int iter = 0;

	uintptr_t curr = ptr;
	bool up = true;

	/**
	 * radiating scan, expect some cache trashing.
	 * however for this one it is worth it.
	 */
start_r:
	if (iter >= max_iter) {
		pr_info("%s: bootsrap fail! iter: %d\n", __func__, iter);
		return -ERANGE;
	}

	if (!!copy_from_kernel_nofault(&ibuf, (void *)curr, sizeof(uint32_t)))
		goto inc;

	if (sprint_symbol(buf, curr) < sizeof(sym))
		goto inc;

	if (strstarts(buf, sym))
		goto found;

inc:
	if (up) {
		iter++;
		curr = anchor + sizeof(uint32_t) * iter;
		up = false;
	} else {
		curr = anchor - sizeof(uint32_t) * iter;
		up = true;
	}

	goto start_r;

found:
	pr_info("%s: %s found at 0x%lx iter: %d\n", __func__, buf, curr, iter);
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
