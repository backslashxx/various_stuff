#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>

// zig cc -target aarch64-linux -Oz -s -static prctl_bench.c -Wl,--gc-sections -o bench
// taskset -c 0 ./bench

#define N_ITERATIONS 250000

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

__attribute__((hot, always_inline, no_stack_protector))
static inline void prctl_call(unsigned long option) {
	volatile unsigned long dummy = 0;
	syscall(SYS_prctl, option, (unsigned long)&dummy,
			(unsigned long)&dummy, (unsigned long)&dummy, (unsigned long)&dummy);
	asm volatile("" ::: "memory"); // barrier();
	//asm volatile("nop");
}

int main() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	unsigned long long t_start, t_end;
	unsigned long long total_kernelsu_option_time = 0;

	unsigned long long total_nop_prctl_time = 0;
	unsigned long long t_start_nop, t_end_nop;


	int k = 0;
	t_start_nop = time_now_ns();
	do {
		prctl_call((unsigned long)0xFFFFFFFF);
		k++;
	} while (k < N_ITERATIONS);
	t_end_nop = time_now_ns();
	total_nop_prctl_time = t_end_nop - t_start_nop;


	int j = 0;
	do {
		//prctl_call((unsigned long)0xDEADBEED);
		prctl_call((unsigned long)0xDEADBEEE);
		j = j + 1;
	} while (j < N_ITERATIONS);

	int i = 0;
	t_start = time_now_ns();
	do {
		prctl_call((unsigned long)0xDEADBEEF);
		i = i + 1;
	} while (i < N_ITERATIONS);
	t_end = time_now_ns();
	total_kernelsu_option_time = t_end - t_start;
	
	long long avg_nop_prctl_ns = (long long)(total_nop_prctl_time / N_ITERATIONS);
	long long avg_real_ns = (long long)(total_kernelsu_option_time / N_ITERATIONS);
	
	double ratio = (double)avg_real_ns / avg_nop_prctl_ns;
	double percent_overhead = (ratio - 1.0) * 100.0;

	printf("iterations: %d\n", N_ITERATIONS);
	printf("average 0xFFFFFFFF: %lld ns\n", avg_nop_prctl_ns);
	printf("average 0xDEADBEEF: %lld ns\n", avg_real_ns);
	printf("0xDEADBEEF/0xFFFFFFFF: %.2f%%\n", percent_overhead);

	if (percent_overhead >= 3.5)
		printf("confidence: very high\n");
	else if (percent_overhead >= 3.0)
		printf("confidence: high\n");
	else if (percent_overhead >= 2.0)
		printf("confidence: maybe\n");
	else if (percent_overhead > 1.0)
		printf("confidence: low\n");
	else
		printf("verdict: try again\n");

	return 0;
}
