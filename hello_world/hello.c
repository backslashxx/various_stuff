#include "small_rt.h"

__attribute__((used))
void prep_main(long *sp)
{
	char hw[] = "Hello world!\n";
	__syscall(SYS_write, 1, (long)hw, sizeof(hw), NONE, NONE, NONE);
	__syscall(SYS_exit, 0, NONE, NONE, NONE, NONE, NONE);
	__builtin_unreachable();
}
