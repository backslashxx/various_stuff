#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

// zig cc -target aarch64-linux -Oz -s -static prctl_bench.c -Wl,--gc-sections -o bench

#define N_ITERATIONS 25000
#define N_FAKE 9

// https://github.com/wtarreau/mhz/blob/master/mhz.c
__attribute__((hot, always_inline))
static inline unsigned long long time_now_ns() {
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

__attribute__((noinline, no_stack_protector))
static void prctl_call(unsigned long option) {
	volatile unsigned long dummy = 0;
	syscall(SYS_prctl, option, (unsigned long)&dummy,
			(unsigned long)&dummy, (unsigned long)&dummy, (unsigned long)&dummy);
	asm volatile("" ::: "memory"); // barrier();
}

int main() {
	unsigned long long t_start, t_end;
	unsigned long long total_fake_time = 0, total_kernelsu_option_time = 0;
	unsigned long long fake_option = 0, kernelsu_option = 0;

	int i = 0;
	do {
		int j = 0;
		do {
			t_start = time_now_ns();
			prctl_call((unsigned long)0xDEADBEED);
			prctl_call((unsigned long)0xDEADBEEE);
			t_end = time_now_ns();
			total_fake_time = total_fake_time + (t_end - t_start);
			fake_option = fake_option + 2;
			j = j + 1;
		} while (j < N_FAKE);

		t_start = time_now_ns();
		prctl_call((unsigned long)0xDEADBEEF);
		t_end = time_now_ns();
		total_kernelsu_option_time = total_kernelsu_option_time + (t_end - t_start);
		kernelsu_option++;

		i = i + 1;
	} while (i < N_ITERATIONS);

	// printf("total_fake_time: %llu ns, fake_option: %llu iter\n", total_fake_time, fake_option);
	// printf("total_kernelsu_option_time: %llu ns, kernelsu_option: %llu iter\n", total_kernelsu_option_time, kernelsu_option);

	long long avg_fake_ns = (long long)(total_fake_time / fake_option); // 0xDEADBEED + 0xDEADBEEE
	long long avg_real_ns = (long long)(total_kernelsu_option_time / kernelsu_option);
	long long overhead_ns = avg_real_ns - avg_fake_ns;

	printf("iterations: %d\n", N_ITERATIONS);
	printf("average !0xDEADBEEF: %lld ns\n", avg_fake_ns);
	printf("average 0xDEADBEEF: %lld ns\n", avg_real_ns);
	printf("off by: %lld ns\n", overhead_ns);

	return 0;
}
