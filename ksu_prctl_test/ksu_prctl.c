#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

	if (3 != argc) {
		printf("Usage: %s option arg2\n", argv[0]);
		return 1;
	}

	unsigned long option = strtoul(argv[1], NULL, 0);
	unsigned long arg2 = strtoul(argv[2], NULL, 0);
	unsigned long arg3 = 0;
	unsigned long arg4 = 0;
	unsigned long arg5 = 0;

	printf("SYS_prctl(%lu, %lu, %p, %p, %p)\n", option, arg2, &arg3, &arg4, &arg5);
	
	syscall(SYS_prctl, option, arg2, &arg3, &arg4, &arg5);

	printf("arg3: %lu\n", arg3);
	printf("arg4: %lu\n", arg4);
	printf("arg5: 0x%lx\n", arg5);

	return 0;

}
