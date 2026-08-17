#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

// /tmp/optane/ndk/android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -flto -fmerge-all-constants su.c -static
// /tmp/optane/ndk/android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi16-clang -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -flto -fmerge-all-constants su.c -static
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip -z a.out

#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)
int main(int argc, char **argv, char **envp)
{
	const char *data_adb_ksud = "/data/adb/ksud";
	const char *system_bin_sh = "/system/bin/sh";	
	argv[0] = "su";

	int fd = 0;
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (long)&fd);
	if (!fd)
		goto trap;

	int ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0);
	if (ret < 0)
		goto trap;

	syscall(SYS_execve, (long)data_adb_ksud, (long)argv, (long)envp);
	syscall(SYS_execve, (long)system_bin_sh, (long)argv, (long)envp);

trap:
	__builtin_trap();
	return 0;
}

