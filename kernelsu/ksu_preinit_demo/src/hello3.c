#include <sys/syscall.h>
#include <fcntl.h>

#include "small_rt.h"
#define syscall __syscall

// zig cc -target aarch64-linux -std=gnu23 -Oz -flto -s -fno-unwind-tables -fmerge-all-constants -ffunction-sections -fdata-sections -fvisibility=hidden -Wl,--entry=__start -s -Wl,--gc-sections,-z,norelro -Wno-int-conversion -Wno-unknown-attributes -Wno-macro-redefined hello3.c
// sstrip a.out

#define strlen __builtin_strlen

int main(int argc, char **argv, char **envp)
{
	constexpr char kmsg[] = "/dev/kmsg";
	int fd = syscall(SYS_openat, AT_FDCWD, (long)kmsg, O_WRONLY, 0);
	if (!(fd >= 0))
		return 0;

	constexpr char msg[] = "ksu_preinit_demo: Hello world!\n";
	syscall(SYS_write, fd, (long)msg, sizeof(msg));

	int i = 0;
	while(argv[i]) {
		syscall(SYS_write, fd, (long)argv[i], strlen(argv[i]));
		i++;
	}

	i = 0;
	while(envp[i]) {
		syscall(SYS_write, fd, (long)envp[i], strlen(envp[i]));
		i++;
	}

	constexpr char msg2[] = "ksu_preinit_demo: print shit on dmesg!\n";
	syscall(SYS_write, fd, (long)msg2, sizeof(msg2));

	return 0;
}
