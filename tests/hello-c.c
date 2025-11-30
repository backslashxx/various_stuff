#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

void add_byref_to_char(int *restrict a, int *restrict b, char *buf)
{
	int x = *a + *b;
	buf[0] = 48 + x;
	buf[1] =  '\n';
}

int main()
{
	int a = 4;
	int b = 5;
	char buf[2];
	
	add_byref_to_char(&a, &b, buf);
	syscall(SYS_write, 1, buf, 2);

	return 0;
}
