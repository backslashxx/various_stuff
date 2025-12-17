#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct __attribute__((packed)) { uint8_t buf[3]; } int24_t;

// assumes LE
int main()
{
	uint8_t buf[4];	
	int24_t x;

	printf("%zu\n", sizeof(int24_t));
	
	// init
	memset(&x, 0, 3);
	memset(&buf, 0, 4);

	// read
	memcpy(&buf, &x, 3);
	printf("%d\n", *(int32_t *)&buf);

	// increment
	int32_t y = 100100;
	memcpy(&x, &y, 3);

	memcpy(&buf, &x, 3);
	printf("%d\n", *(int32_t *)&buf);

	// replace
	y = 10501;
	memset(&x, 0, 3);
	memcpy(&x, &y, 3);

	memset(&buf, 0, 4);
	memcpy(&buf, &x, 3);
	
	printf("%d\n", *(int32_t *)&buf);

	return 0;
}
