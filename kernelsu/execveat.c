#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, const char **argv, const char **envp)
{
	argv[0] = "su";
	syscall(SYS_execveat, AT_FDCWD, "/system/bin/su", argv, envp, 0);
	__builtin_trap();
	return 0;
	
}
