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

#define N_ITERATIONS 1000
#define N_BATCH 8

// https://github.com/wtarreau/mhz/blob/master/mhz.c
__attribute__((hot, always_inline))
static unsigned long long time_now_ns() {
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

__attribute__((hot, always_inline, no_stack_protector))
static void prctl_call(unsigned long option) {
	volatile unsigned long dummy = 0;
	syscall(SYS_prctl, option, (unsigned long)&dummy,
			(unsigned long)&dummy, (unsigned long)&dummy, (unsigned long)&dummy);
	asm volatile("" ::: "memory"); // barrier();
	//asm volatile("nop");
}

// https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
static int compare_ull(const void *arg1, const void *arg2) {
	unsigned long long ull_a = *(unsigned long long *)arg1;
	unsigned long long ull_b = *(unsigned long long *)arg2;
	// truth table
	// a > b ; 1 - 0
	// a = b ; 0 - 0
	// a < b ; 0 - 1
	return (ull_a > ull_b) - (ull_a < ull_b);
}

int main() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	unsigned long long deadbeef_samples[N_ITERATIONS];
	unsigned long long ffffffff_samples[N_ITERATIONS];

	unsigned long long total_deadbeef_time = 0;
	unsigned long long total_ffffffff_time = 0;
	
//	int loops = 0;
//start:
	/* waza for FFFFFFFF */
	int j = 0;
	do {
		int z = 0;
		do {
			prctl_call((unsigned long)0xFFFFFFFF);
			prctl_call((unsigned long)0xFFFFFFFF);
			z = z +1 ;
		} while (z < N_BATCH);
		j = j + 1;
		asm volatile("nop");
	} while (j < 250);

	int k = 0;
	do {
		unsigned long long t0 = time_now_ns();
		int z = 0;
		do {
			prctl_call((unsigned long)0xFFFFFFFF);
			z = z + 1;
		} while (z < N_BATCH);	
		unsigned long long t1 = time_now_ns();
		ffffffff_samples[k] = (t1 - t0) / N_BATCH;
		total_ffffffff_time += ffffffff_samples[k];
		k = k + 1;
		asm volatile("nop");
	} while (k < N_ITERATIONS);

	/* waza for DEADBEEF */
	j = 0;
	do {
		int z = 0;
		do {
			prctl_call((unsigned long)0xDEADBEED);
			prctl_call((unsigned long)0xDEADBEEE);
			z = z +1 ;
		} while (z < N_BATCH);
		j = j + 1;
		asm volatile("nop");
	} while (j < 250);

	int i = 0;
	do {
		int z = 0;
		unsigned long long t0 = time_now_ns();
		do {
			prctl_call((unsigned long)0xDEADBEEF);
			z = z + 1;
		} while (z < N_BATCH);
		unsigned long long t1 = time_now_ns();
		deadbeef_samples[i] = (t1 - t0) / N_BATCH;
		total_deadbeef_time += deadbeef_samples[i];
		i = i + 1;
		asm volatile("nop");
	} while (i < N_ITERATIONS);
	
//	loops = loops + 1;
//	if (!(loops > N_BATCH))
//		goto start;

	long long avg_real_ns = total_deadbeef_time / N_ITERATIONS;
	long long avg_nop_prctl_ns = total_ffffffff_time / N_ITERATIONS;
	
	double ratio = (double)avg_real_ns / avg_nop_prctl_ns;
	double percent_overhead = (ratio - 1.0) * 100.0;

	// sort the array we got
	qsort(deadbeef_samples, N_ITERATIONS, sizeof(deadbeef_samples[0]), compare_ull);
	qsort(ffffffff_samples, N_ITERATIONS, sizeof(ffffffff_samples[0]), compare_ull);

	unsigned long long median_deadbeef;
	unsigned long long median_ffffffff;

	// grab that shit in the middle
	median_deadbeef = deadbeef_samples[N_ITERATIONS / 2];
	median_ffffffff = ffffffff_samples[N_ITERATIONS / 2];
	
	double ratio_overhead = (double)median_deadbeef /(double)median_ffffffff;
	double median_overhead = (ratio_overhead - 1.0) * 100.0;

	printf("iterations: %d\n", N_ITERATIONS);
	printf("average 0xFFFFFFFF: %lld ns\n", avg_nop_prctl_ns);
	printf("average 0xDEADBEEF: %lld ns\n", avg_real_ns);
	printf("average 0xDEADBEEF/0xFFFFFFFF: %.2f%%\n", percent_overhead);
	printf("median 0xFFFFFFFF: %llu ns\n", median_ffffffff);
	printf("median 0xDEADBEEF: %llu ns\n", median_deadbeef);
	printf("median 0xDEADBEEF/0xFFFFFFFF: %.2f%%\n", median_overhead);

	if (median_overhead >= 1.5) {
		if (percent_overhead >= 1.5)
			printf("confidence: high\n");
		else if (percent_overhead > 0.5)
			printf("confidence: maybe\n");
		else
			printf("verdict: try again\n");
	} else
		printf("verdict: try again\n");

	return 0;
}
