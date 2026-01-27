#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

/* Show the contents of the symbolic links in /proc/self/fd */

static void
show_fds(void)
{
	DIR	 *dirp;
	char	path[PATH_MAX], target[PATH_MAX];
	ssize_t len;
	struct dirent  *dp;

	dirp = opendir("/proc/self/fd");
	if (dirp  == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	for (;;) {
		dp = readdir(dirp);
		if (dp == NULL)
			break;

		if (dp->d_type == DT_LNK) {
			snprintf(path, sizeof(path), "/proc/self/fd/%s", dp->d_name);

			len = readlink(path, target, sizeof(target));
			printf("%s ==> %.*s\n", path, (int) len, target);
		}
	}

	closedir(dirp);
}

int
main(int argc, char *argv[])
{
	int  fd;

	for (size_t j = 1; j < argc; j++) {
		fd = open(argv[j], O_RDONLY);
		if (fd == -1) {
			perror(argv[j]);
			exit(EXIT_FAILURE);
		}
		printf("%s opened as FD %d\n", argv[j], fd);
	}

	show_fds();

	printf("========= About to call close_range() =======\n");


// __arm64_sys_close_range unsigned int fd, unsigned int max_fd, unsigned int flags

	if (syscall(SYS_close_range, 3, ~0U, 0) == -1) {
		perror("close_range");
		exit(EXIT_FAILURE);
	}

	show_fds();
	exit(EXIT_FAILURE);
}

