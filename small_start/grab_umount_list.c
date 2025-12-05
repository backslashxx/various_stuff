#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <stdint.h>

// zig cc -target aarch64-linux small_start.c -Oz -s -static -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -Wno-int-conversion -o small_start 

#define NONE 0

// syscall de-wrappers
#if defined(__aarch64__)
__attribute__((noinline))
static long __syscall(long n, long a, long b, long c, long d, long e, long f)
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

#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE

struct ksu_add_try_umount_cmd {
	uint64_t arg; // char ptr, this is the mountpoint
	uint32_t flags; // this is the flag we use for it
	uint8_t mode; // denotes what to do with it 0:wipe_list 1:add_to_list 2:delete_entry
};
#define KSU_UMOUNT_GETSIZE 107   // get list size
#define KSU_UMOUNT_GETLIST 108   // get list

#define KSU_IOCTL_ADD_TRY_UMOUNT _IOC(_IOC_WRITE, 'K', 18, 0)

#define alloca __builtin_alloca
#define memset __builtin_memset
#define strlen __builtin_strlen

static int c_main(int argc, char **argv, char **envp)
{
	const char *newline = "\n";
	unsigned long total_size = 0;
	int fd = 0;

	__syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (long)&fd, NONE, NONE);
	if (!fd)
		return 1;

	struct ksu_add_try_umount_cmd cmd = {0};
	cmd.arg = (uint64_t)&total_size;
	cmd.flags = 0;
	cmd.mode = KSU_UMOUNT_GETSIZE;


	int ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_ADD_TRY_UMOUNT, (long)&cmd, NONE, NONE, NONE);
	if (ret < 0)
		return 1;

	if (!total_size)
		return 1;

	// now we can prepare the same size of memory		
	void *buffer = alloca(total_size);
	if (!buffer)
		return 1;
	
	memset(buffer, 0, total_size);

	cmd.arg = (uint64_t)buffer;
	cmd.flags = 0;
	cmd.mode = KSU_UMOUNT_GETLIST;

	ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_ADD_TRY_UMOUNT, (long)&cmd, NONE, NONE, NONE);
	if (ret < 0)
		return 1;

	// now we pointerwalk
	const char *char_buf = (const char *)buffer;
	do {
		//printf("list_entry: %s \n", char_buf);
		
		__syscall(SYS_write, 1, char_buf, strlen(char_buf), NONE, NONE, NONE);
		__syscall(SYS_write, 1, newline, 1, NONE, NONE, NONE);
		
		char_buf = char_buf + strlen(char_buf) + 1;
	} while (*char_buf);

	return 0;

}

void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = (char **)(sp + 2);

	long exit_code = c_main(argc, argv, envp);
	__syscall(SYS_exit, exit_code, NONE, NONE, NONE, NONE, NONE);
}

#if 0
// -fno-omit-frame-pointer ?
void __start(void) {
	long *sp = (long*)__builtin_frame_address(0);
	prep_main(sp);
}
#endif

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
