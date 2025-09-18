#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (3 != argc) {
		printf("Usage: %s <from_path> <to_path>\n", argv[0]);
		return 1;
	}

	const char *from = argv[1];
	const char *to   = argv[2];

	int ret = syscall(SYS_move_mount, AT_FDCWD, from, AT_FDCWD, to, 0);
	if (ret < 0) {
		printf("!!\n");
		return 1;
	}

	printf("sys_move_mount ok: %s -> %s\n", from, to);
	return 0;
}
