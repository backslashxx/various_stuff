#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

	if (6 != argc) {
		printf("Usage: %s option arg2 arg3 arg4 arg5\n", argv[0]);
		return 1;
	}

/* 

add this to ksu's prctl interface, then call me like
./prctl 0xDEADBEEF 0xAABBCCDD 10000 0 0

change 10000 to uid you want to set
grab uid from 
grep "weishu" /data/system/packages.list


	#define CMD_SET_MANAGER_UID 0xAABBCCDD
	if (arg2 == CMD_SET_MANAGER_UID) {

		uid_t new_uid;
		
		if (copy_from_user(&new_uid, arg3, sizeof(new_uid))) {
			pr_err("copy new uid failed\n");
			return 0;
		}
		
		ksu_set_manager_uid(new_uid);
		pr_info("new manager uid: %d\n", new_uid);

		if (copy_to_user(result, &reply_ok, sizeof(reply_ok))) {
			pr_err("prctl reply error, cmd: %lu\n", arg2);
		}

		return 0;
	}



*/


	unsigned long option = strtoul(argv[1], NULL, 0);
	unsigned long arg2 = strtoul(argv[2], NULL, 0);
	unsigned long arg3 = strtoul(argv[3], NULL, 0);
	unsigned long arg4 = strtoul(argv[4], NULL, 0);
	unsigned long arg5 = strtoul(argv[5], NULL, 0);

	printf("SYS_prctl(%lu, %lu, %p, %p, %p)\n", option, arg2, &arg3, &arg4, &arg5);
	
	syscall(SYS_prctl, option, arg2, &arg3, &arg4, &arg5);

	printf("arg3: %lu\n", arg3);
	printf("arg4: %lu\n", arg4);
	printf("arg5: 0x%lx\n", arg5);

	return 0;

}
