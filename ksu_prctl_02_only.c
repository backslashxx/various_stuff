#include <sys/syscall.h>
#include <unistd.h>

int main() {

	unsigned long arg3 = 0;
	unsigned long arg4 = 0;
	unsigned long arg5 = 0;

	syscall(SYS_prctl, (unsigned long)0xdeadbeef, (unsigned long)2, &arg3, &arg4, &arg5);

	if (arg3 > 12000)
		syscall(SYS_write, 1, "success\n", 8);
	else
		syscall(SYS_write, 2, "fail\n", 5);
	
	return 0;
}
