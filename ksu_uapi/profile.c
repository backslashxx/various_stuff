#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>

#include "uapi_profile.h"

#define KSU_INSTALL_MAGIC1 0xDEADBEEF
#define KSU_INSTALL_MAGIC2 0xCAFEBABE

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

// get uid from kernelsu
struct ksu_get_manager_uid_cmd {
	uint32_t uid;
};
#define KSU_IOCTL_GET_MANAGER_UID _IOC(_IOC_READ, 'K', 10, 0)

#define KSU_IOCTL_GET_APP_PROFILE _IOC(_IOC_READ | _IOC_WRITE, 'K', 11, 0)
#define KSU_IOCTL_SET_APP_PROFILE _IOC(_IOC_WRITE, 'K', 12, 0)

int main(int argc, char **argv, char **envp)
{
	struct ksu_get_info_cmd buf = { 0 };
	struct app_profile profile = { 0 };
	int fd = 0;

	if (syscall(SYS_getuid)) {
		printf("Error: need root!\n");
		return 1;
	}

	syscall(SYS_reboot, KSU_INSTALL_MAGIC1, KSU_INSTALL_MAGIC2, 0, &fd);
	if (!fd)
		return 1;

	int ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GET_INFO, (long)&buf);
	if (ret) {
		// if new ioctl fails, we try again, this time trying legacy
		ret = syscall(SYS_ioctl, KSU_IOCTL_GET_INFO_LEGACY, (long)&buf);
		if (ret)
			printf("Error: KernelSU not installed?\n");
		else
			printf("Error: KernelSU too old!\n");
			

		return 1;
	}

	// printf("uapi_version: %d \n", buf.uapi_version);

	if (argv[1] && !strcmp(argv[1], "--getuidinfo") && argv[2])
		goto getuidinfo;

	if (argv[1] && !strcmp(argv[1], "--setumount") && argv[2] && argv[3])
		goto setumount;

	// help here
	printf("%s --getuidinfo 12345\t to print info on app with uid\n", argv[0]);
	printf("%s --setumount 12345 1/0\t to toggle umount for app with uid\n", argv[0]);

	return 0;

getuidinfo:
	;
	// get current manager uid
	struct ksu_get_manager_uid_cmd cmd = { 0 };
	ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GET_MANAGER_UID, (long)&cmd);
	if (ret || !cmd.uid) {
		printf("Error: Manager not installed!\n");
		return 1;
	}

	// printf("manager_uid: %d \n", cmd.uid);

	// TODO: isidigit loop
	int32_t req_uid = atoi(argv[2]);
	if (req_uid < 10000) {
		printf("Error: bad uid!\n");
		return 1;
	}

	// printf("requested uid: %d\n", req_uid);

	if (!!syscall(SYS_setuid, cmd.uid)) {
		printf("Error: setuid fail!\n");
		return 1;
	}

	profile.version = KSU_APP_PROFILE_VER;
	profile.curr_uid = req_uid;

	if (!!syscall(SYS_ioctl, fd, KSU_IOCTL_GET_APP_PROFILE, (long)&profile)) {
		printf("Error: no profile!\n");
		return 1;
	}

	if (!profile.key[0]) {
		printf("Error: no profile!\n");
		return 1;
	}

	printf("profile info\n");
	printf("pkg_name: \t%s\n", profile.key);
	printf("uid: \t\t%d\n", profile.curr_uid);

	if (profile.allow_su) {
		printf("su: \t\tyes\n");
		printf("use_default: \t%s\n", profile.rp_config.use_default ? "yes" : "no");
		// printf("template: \t%s\n", profile.rp_config.template_name);
		// printf("selinux_domain: \t%s\n", profile.rp_config.profile.selinux_domain);
		printf("root uid/gid: \t%d/%d\n", profile.rp_config.profile.uid, profile.rp_config.profile.gid);
	} else {
		printf("su: \t\tno\n");	
		printf("use_default: \t%s\n", profile.nrp_config.use_default ? "yes" : "no");
		printf("umount: \t%s\n", profile.nrp_config.profile.umount_modules ? "true" : "false");
	}

	return 0;

setumount:
	;

	ret = syscall(SYS_ioctl, fd, KSU_IOCTL_GET_MANAGER_UID, (long)&cmd);
	if (ret || !cmd.uid) {
		printf("Error: Manager not installed!\n");
		return 1;
	}

	req_uid = atoi(argv[2]);
	if (req_uid < 10000) {
		printf("Error: bad uid!\n");
		return 1;
	}

	if (!!syscall(SYS_setuid, cmd.uid)) {
		printf("Error: setuid fail!\n");
		return 1;
	}

	profile.version = KSU_APP_PROFILE_VER;
	profile.curr_uid = req_uid;

	if (!!syscall(SYS_ioctl, fd, KSU_IOCTL_GET_APP_PROFILE, (long)&profile)) {
		printf("Error: no profile!\n");
		return 1;
	}

	if (!profile.key[0]) {
		printf("Error: no profile!\n");
		return 1;
	}

	if (profile.allow_su) {
		printf("Error: umount for SU app is nono!\n");
		return 1;
	}

	// enable umount here
	profile.nrp_config.use_default = false;
	profile.nrp_config.profile.umount_modules = !!atoi(argv[3]);

	if (!!syscall(SYS_ioctl, fd, KSU_IOCTL_SET_APP_PROFILE, (long)&profile)) {
		printf("Error: set umount fail!\n");
		return 1;
	}

	printf("Success: umount set to %d app with uid: %d \n", !!atoi(argv[3]), req_uid);


	return 0;



}
