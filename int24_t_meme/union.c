#include <stdio.h>
#include <stdint.h>
#include <string.h>


union sulog {
	uint32_t uid;
	uint8_t sym;
};

int main()
{
	union sulog n = {0};

	*(char *)&n = 'x';
	n.uid = 1100400;

	printf("%zu - %c - %d\n", sizeof(n), *(char *)&n, (uint32_t)n.uid);

	return 0;
}
