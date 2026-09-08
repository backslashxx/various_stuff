#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include "small_rt.h"

__attribute__((always_inline))
static int dumb_atoi(const char *str)
{
	int res = 0;

start:
	// llvm actually has an optimized isdigit
	// just not prefixed with __builtin
	// code generated is the same size, so better use it
	if (!isdigit(*str))
		return INT_MAX;

	res = (res * 10) + (*str - 48);
	str++;

	if (*str)
		goto start;

	return res;
}

char str1[] = "syscall exists!\n";
char str2[] = "syscall does NOT exist!\n";
char str3[] = "erronous input!\ntry ./sctest 123\n";

__attribute__((always_inline))
int c_main(int argc, char *argv[], char *envp[])
{
	char *str = str3;
	long sz = sizeof(str3);
	long sc = 0;

	if (!argv[1])
		goto fail;

	if (argv[1])
		sc = dumb_atoi(argv[1]);

	if (sc == INT_MAX)
		goto fail;

	int ret = __syscall(sc, NONE, NONE, NONE, NONE, NONE, NONE);
	if (ret == -ENOSYS)
		goto enosys;

	str = str1;
	sz = sizeof(str1);
	goto print_out;

enosys:
	str = str2;
	sz = sizeof(str2);
fail:
print_out:
	__syscall(SYS_write, 1, (long)str, sz, NONE, NONE, NONE);
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
