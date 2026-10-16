#include <sys/syscall.h>
#include "small_rt.h"

#ifndef AT_FDCWD
#define AT_FDCWD -100
#endif

#ifndef O_WRONLY
#define O_WRONLY 1
#endif

__attribute__((used))
void prep_main(long *sp)
{
	char path[] = "/dev/kmsg";
	long fd = __syscall(SYS_openat, AT_FDCWD, (long)path, O_WRONLY, 0, NONE, NONE);
	if (fd >= 0) {
		char msg[] = "TESTING: hello from security_sb_mount\n";
		__syscall(SYS_write, fd, (long)msg, sizeof(msg) - 1, NONE, NONE, NONE);
	}

	__syscall(SYS_exit, 0, NONE, NONE, NONE, NONE, NONE);
	__builtin_unreachable();
}
