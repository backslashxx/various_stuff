#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// argv1 = sec, argv2 =  nsec

int main(int argc, char *argv[]) {
	if (argc < 2)
		__builtin_trap();

	int epfd = epoll_create1(0);
	struct epoll_event events[1];
	
	uint64_t sec = strtoul(argv[1], NULL, 0);
	uint64_t nsec = strtoul(argv[2], NULL, 0);

	struct timespec timeout = { .tv_sec = sec, .tv_nsec = nsec };

	int n = syscall(SYS_epoll_pwait2, epfd, events, 1, &timeout, NULL);

	if (n == 0)
		printf("done\n");

	return 0;
}
