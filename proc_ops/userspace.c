#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <stdint.h>

struct uint64_tx7 {
	uint64_t magic;
	uint64_t r0;
	uint64_t r1;
	uint64_t r2;
	uint64_t r3;
	uint64_t r4;
	uint64_t r5;
	uint64_t r6;
};

int fd = 0;

int shitcall(uint64_t r0, uint64_t r1, uint64_t r2, uint64_t r3, uint64_t r4, uint64_t r5, uint64_t r6)
{
	// always zero init, ala xzr
	struct uint64_tx7 reg = { 0 };
	reg.magic = 0xFFFFFFFFFFFFFFFF;
	reg.r0 = r0;
	reg.r1 = r1;
	reg.r2 = r2;
	reg.r3 = r3;
	reg.r4 = r4;
	reg.r5 = r5;
	reg.r6 = r6;

	syscall(SYS_write, fd, (unsigned long)&reg, 1);
	if (reg.magic != 0xAAAAAAAAAAAAAAAA)
		return 1;
	
	return 0;
}

int main(int argc, char **argv)
{
	fd = syscall(SYS_openat, AT_FDCWD, "/proc/kallsyms", O_WRONLY, 0644);
	if (fd < 0)
		goto error;

	int ret = shitcall(0, 1, 2, 3, 4, 5, 6);
	if (ret)
		goto error;

	syscall(SYS_exit, 0);
	__builtin_unreachable();

error:
	syscall(SYS_exit, 1);
	__builtin_unreachable();
	return 0; // dummy
}
