#include "small_rt.h"
// zig cc -target aarch64-linux small_start.c -Oz -s -static -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -Wno-int-conversion -o small_start 

// youre in C at this point
static int c_main(int argc, char **argv, char **envp)
{

	// small argc / argv demo
	int i = 0;
	const char *newline = "\n";
	
argv_loop_start:
	char *arg = argv[i];
	int len = 0;
	while(arg[len])
		len++;

	__syscall(SYS_write, 1, (long)arg, len, NONE, NONE, NONE);
	__syscall(SYS_write, 1, (long)newline, 1, NONE, NONE, NONE);

	i++;

	if (argv[i])
		goto argv_loop_start;

	i = 0;

envp_loop_start:
	char *env = envp[i];
	len = 0;
	while(env[len])
		len++;

	__syscall(SYS_write, 1, (long)env, len, NONE, NONE, NONE);
	__syscall(SYS_write, 1, (long)newline, 1, NONE, NONE, NONE);

	i++;
	
	if (env[i])
		goto envp_loop_start;

	return 0;
}

void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = argv + argc + 1; // we need to offset it by the number of argc's!

	long exit_code = c_main(argc, argv, envp);
	__syscall(SYS_exit, exit_code, NONE, NONE, NONE, NONE, NONE);
}
