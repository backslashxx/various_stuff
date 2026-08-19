#include <sys/syscall.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#if defined(__arm__) 
#define SYS_newfstatat SYS_fstatat64
#endif

int main(int argc, const char **argv, const char **envp)
{

	struct stat st;
	argv[0] = "su";

	syscall(SYS_faccessat, AT_FDCWD, "/system/bin/su", F_OK, 0);
	syscall(SYS_newfstatat, AT_FDCWD, "/system/bin/su", (long)&st, AT_SYMLINK_NOFOLLOW);

	syscall(SYS_execveat, AT_FDCWD, "/system/bin/su", argv, envp, 0);
	syscall(SYS_execve, "/system/bin/su", argv, envp);

	__builtin_trap();
	return 0;
	
}
