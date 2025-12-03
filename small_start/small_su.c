#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// zig cc -target aarch64-linux small_su.c -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -Wno-int-conversion -o small_su 

// https://gcc.gnu.org/onlinedocs/gcc/Library-Builtins.html
// https://clang.llvm.org/docs/LanguageExtensions.html#builtin-functions
#define memcmp __builtin_memcmp
#define strlen __builtin_strlen

// ksu's new supercall
#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)

int fd = 0;

// syscall de-wrappers
#if defined(__aarch64__)
__attribute__((noinline))
static long __syscall(long n,long a,long b,long c,long d,long e,long f)
{
	register long 
		x8 asm("x8") = n,
		x0 asm("x0") = a,
		x1 asm("x1") = b,
		x2 asm("x2") = c,
		x3 asm("x3") = d,
		x4 asm("x4") = e,
		x5 asm("x5") = f;

	asm volatile("svc #0"
		:"=r"(x0)
		:"r"(x8), "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
		:"memory");

	return x0;
}

#elif defined(__arm__)
__attribute__((noinline))
static long __syscall(long n, long a, long b, long c, long d, long e, long f) {
	register long
		r7 asm("r7") = n,
		r0 asm("r0") = a,
		r1 asm("r1") = b,
		r2 asm("r2") = c,
		r3 asm("r3") = d,
		r4 asm("r4") = e,
		r5 asm("r5") = f;

	asm volatile("svc #0"
		: "=r"(r0)
		: "r"(r7), "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
		: "memory");

	return r0;
}

#elif defined(__x86_64__)
__attribute__((noinline))
static long __syscall(long n, long a, long b, long c, long d, long e, long f) {
	long ret;
	asm volatile(
		"mov %5, %%r10\n"
		"mov %6, %%r8\n"
		"mov %7, %%r9\n"
		"syscall"
		: "=a"(ret)
		: "a"(n), "D"(a), "S"(b), "d"(c), "r"(d), "r"(e), "r"(f)
		: "rcx", "r11", 
		"memory");

	return ret;
}
#endif

static int c_main(int argc, const char **argv, const char **envp)
{
	const char *error = "Denied\n";
	unsigned long result = 0;
	
	int is_data = !memcmp(argv[0], "/data", strlen("/data"));
	
	if (is_data) {
		// if its called from /data/adb, dont continue!
		goto denied;
	}

	__syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (void *)&fd, 0, 0);
	if (fd == 0) {
		goto denied;
	} else {
		int ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0, 0, 0, 0);
		if (ret < 0)
			goto denied;
	}

	argv[0] = "su";

	char *debug_msg = "KernelSU: kernelnosu su->ksud\n";
	char *kmsg = "/dev/kmsg";
	fd = __syscall(SYS_openat, AT_FDCWD, kmsg, O_WRONLY, 0, 0, 0);
	if (fd >= 0) {
		__syscall(SYS_write, fd, debug_msg, strlen(debug_msg), 0, 0, 0);
		__syscall(SYS_close, fd, 0, 0, 0, 0, 0);
	}

	__syscall(SYS_execve, "/data/adb/ksud", argv, envp, 0, 0, 0);

denied:	
	__syscall(SYS_write, 2, error, strlen(error), 0, 0, 0);
	return 1;

}

void prep_main(long *sp)
{
	long argc = *sp;
	const char **argv = (const char **)(sp + 1);
	const char **envp = (const char **)(sp + 2);

	int call = c_main(argc, argv, envp);
	if (!call)
		__syscall(SYS_exit, 0, 0, 0, 0, 0, 0);
	else
		__syscall(SYS_exit, 1, 0, 0, 0, 0, 0);
}


// arch specific small entry points
#if defined(__aarch64__)
__attribute__((naked))
void __start(void) {
	asm volatile(
		"mov x0, sp\n"
		"b prep_main\n"
	);
}

#elif defined(__arm__)
__attribute__((naked))
void __start(void) {
	asm volatile(
		"mov r0, sp\n"
		"b prep_main\n"
    );
}

#elif defined(__x86_64__)
__attribute__((naked, section(".text.start"))) 
void __start(void) {
	asm volatile(
		"mov %rsp, %rdi\n"
		"jmp prep_main\n"
	);
}
#endif
