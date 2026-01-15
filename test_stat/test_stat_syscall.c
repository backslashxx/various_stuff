#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("usage: %s <file>\n", argv[0]);
		return 1;
	}

	struct stat st;

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		goto bail;

#if defined(__arm__) && !defined(__aarch64__)
	long ret = syscall(SYS_fstat64, fd, &st);
#elif defined(__aarch64__)
	long ret = syscall(SYS_newfstat, fd, &st);
#else
	#error "no"
#endif

	if (ret)
		goto bail;

	printf("file:\t%s\n", argv[1]);
	printf("size:\t%llu bytes\n", (unsigned long long)st.st_size);


bail:
	printf("fail\n");
	return 1;

out:
	return 0;
}
