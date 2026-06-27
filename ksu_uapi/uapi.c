#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

struct ksu_get_info_legacy_cmd {
	uint32_t version; // Output: KERNEL_SU_VERSION
	uint32_t flags; // Output: KSU_GET_INFO_FLAG_* bits
	uint32_t features; // Output: max feature ID supported
};
#define KSU_IOCTL_GET_INFO_LEGACY _IOC(_IOC_READ, 'K', 2, 0)

struct ksu_get_info_cmd {
	uint32_t version; /* Output: KERNEL_SU_VERSION */
	uint32_t flags; /* Output: KSU_GET_INFO_FLAG_* bits */
	uint32_t features; /* Output: max feature ID supported */
	uint32_t uapi_version; /* Output: KERNEL_SU_UAPI_VERSION */
};
#define KSU_IOCTL_GET_INFO _IOR('K', 2, struct ksu_get_info_cmd)

int main()
{
	struct ksu_get_info_cmd buf = { 0 };
	int fd = 0;

	syscall(SYS_reboot, 0xDEADBEEF, 0xCAFEBABE, 0, &fd);
	if (!fd)
		__builtin_trap();
	
	int ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GET_INFO, (long)&buf);
	if (ret) {
		// if new ioctl fails, we try again, this time trying legacy
		ret = syscall(SYS_ioctl, KSU_IOCTL_GET_INFO_LEGACY, (long)&buf);
		if (ret)
			__builtin_trap();
	}

	return *(int *)&buf.uapi_version;
}
