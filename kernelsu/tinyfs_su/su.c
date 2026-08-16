#include "small_rt.h"
#include <sys/ioctl.h>
#include <fcntl.h>

// /tmp/optane/ndk/android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -flto -fmerge-all-constants su.c -static -nostartfiles -ffreestanding
// /tmp/optane/ndk/android-ndk-r23b/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi16-clang -Oz -s -Wl,--gc-sections,--strip-all,-z,norelro -fno-unwind-tables -Wl,--entry=__start -flto -fmerge-all-constants su.c -static -nostartfiles -ffreestanding
// /tmp/optane/openwrt-mr7350/staging_dir/host/bin/sstrip -z a.out

#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE
#define KSU_IOCTL_GRANT_ROOT _IOC(_IOC_NONE, 'K', 1, 0)

__attribute__((always_inline))
static void c_main(long argc, char **argv, char **envp)
{
	const char *data_adb_ksud = "/data/adb/ksud";
	const char *system_bin_sh = "/system/bin/sh";	
	argv[0] = "su";

	int fd = 0;
	__syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (long)&fd, NONE, NONE);
	if (!fd)
		goto trap;

	int ret = __syscall(SYS_ioctl, fd, KSU_IOCTL_GRANT_ROOT, 0, NONE, NONE, NONE);
	if (ret < 0)
		goto trap;

	__syscall(SYS_execve, (long)data_adb_ksud, (long)argv, (long)envp, NONE, NONE, NONE);
	__syscall(SYS_execve, (long)system_bin_sh, (long)argv, (long)envp, NONE, NONE, NONE);

trap:
	__builtin_trap();
}

__attribute__((used))
void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = argv + argc + 1; // we need to offset it by the number of argc's!

	c_main(argc, argv, envp);
	__builtin_unreachable();
}
