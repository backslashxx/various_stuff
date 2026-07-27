#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef SYS_clone3
#define SYS_clone3 435
#endif

int main(int argc, char *argv[])
{
	int n = syscall(SYS_clone3, NULL, NULL);

	printf("syscall ret: %d\n", n);

	return 0;
}
