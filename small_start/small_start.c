#include <sys/syscall.h>
#include <unistd.h>

// zig cc -target aarch64-linux small_start.c -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -o small_start

void c_main(long *sp) {
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	int i = 0;

loop_start:
	if (argc > i) {
		char *arg = argv[i];
		int len = 0;
		while(arg[len])
			len++;

		syscall(SYS_write, 1, arg, len);
		syscall(SYS_write, 1, "\n", 1);
	}

	i++;

	if (i < 10)
		goto loop_start;

	syscall(SYS_exit, 0);
}

__attribute__((naked)) 
void __start(void) {
	__asm__ volatile (
	    "mov x0, sp\n"   // (sp) to x0 (1st argument)
	    "b c_main\n"     // jump to c_main
	);
}
