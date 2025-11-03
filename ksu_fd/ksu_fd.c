#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE

// test
struct ksu_get_info_cmd {
	uint32_t version; // Output: KERNEL_SU_VERSION
	uint32_t flags; // Output: flags (bit 0: MODULE mode)
	uint32_t features; // Output: max feature ID supported
};

#define KSU_IOCTL_GET_INFO _IOC(_IOC_READ, 'K', 2, 0)

int main(void)
{
	int fd = 0; // we get that fd here

	// ask for an fd
	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, (void *)&fd);

	// by default it doesnt do shit, we have to check if we got somethign back
	// since we init this as 0, if its still 0 then it failed
	if (!fd) {
		printf("sys_reboot failed\n");
		return 1;
	}

	printf("[+] fd : %d\n", fd);

	struct ksu_get_info_cmd info = {0};

	int ret = ioctl(fd, KSU_IOCTL_GET_INFO, &info);
	if (ret < 0) {
		printf("[-] KSU_IOCTL_GET_INFO failed\n");
	} else {
		printf("[+] ksuver: %d\n", info.version);
		printf("[+] flags: 0x%x\n", info.flags);
		printf("[+] features: 0x%x\n", info.features); 
	}

	// close fd when done
	close(fd);
	return 0;
}
