#include <sys/syscall.h>
#include <unistd.h>

int main() {

	unsigned long arg3 = 0;
	unsigned long arg4 = 0;
	unsigned long arg5 = 0;

	syscall(SYS_prctl, (unsigned long)0xdeadbeef, (unsigned long)2, &arg3, &arg4, &arg5);

	if (arg3 > 10000) {
		// dumb long to char
		char digits[6];
		int i = 4;
		arg3 = arg3 % 100000; // oob filter
		do {
			digits[i] = 48 + (arg3 % 10);
			arg3 = arg3 / 10;
			i--;			
		} while (i >= 0);

		digits[5] = '\n';
		syscall(SYS_write, 1, digits, 6);
	} else
		syscall(SYS_write, 2, "fail\n", 5);

	return 0;
}
