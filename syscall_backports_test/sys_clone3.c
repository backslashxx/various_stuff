#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#include "small_rt.h"

#ifndef SYS_clone3
#define SYS_clone3 435
#endif

int c_main(int argc, char *argv[], char *envp[])
{
	char *str = "syscall exists!\nsyscall does NOT exist!\n";

	int n = __syscall(SYS_clone3, NULL, NULL, NONE, NONE, NONE, NONE);

	if (n == -ENOSYS)
		str = str + 16;

	__syscall(SYS_write, 1, (long)str, 24, NONE, NONE, NONE);

	return 0;
}

__attribute__((used))
void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = argv + argc + 1; // we need to offset it by the number of argc's!

	long exit_code = c_main(argc, argv, envp);
	__syscall(SYS_exit, exit_code, NONE, NONE, NONE, NONE, NONE);
	__builtin_unreachable();
}
