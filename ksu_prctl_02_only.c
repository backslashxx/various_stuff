#include <sys/syscall.h>
#include <unistd.h>

int main() {

	unsigned long arg3 = 0;
	unsigned long arg4 = 0;
	unsigned long arg5 = 0;

	syscall(SYS_prctl, (unsigned long)0xdeadbeef, (unsigned long)2, &arg3, &arg4, &arg5);

	if (arg3 > 10000) {
		// dumb long to char
		char a = (arg3 % 100000) / 10000;
		char b = (arg3 - a * 10000) / 1000;
		char c = (arg3 - a * 10000 - b * 1000) / 100;
		char d = (arg3 - a * 10000 - b * 1000 - c * 100) / 10;
		char e = (arg3 - a * 10000 - b * 1000 - c * 100 - d * 10);

		char digits[6];
		digits[0] = 48 + a;
		digits[1] = 48 + b;
		digits[2] = 48 + c;
		digits[3] = 48 + d;
		digits[4] = 48 + e;
		digits[5] = '\n';
		syscall(SYS_write, 1, digits, 6);
	} else
		syscall(SYS_write, 2, "fail\n", 5);

	return 0;
}
