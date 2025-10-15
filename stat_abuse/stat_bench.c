#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // For AT_FDCWD

// zig cc -target aarch64-linux -Oz -s -static stat_bench.c -Wl,--gc-sections -o bench
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
static void newfstatat_call() {
	struct stat st;
	syscall(SYS_newfstatat, AT_FDCWD, (int)0, &st, 0);
	asm volatile("" ::: "memory"); // barrier();
}

__attribute__((hot, always_inline, no_stack_protector))
static void newfstat_call() {
	struct stat st;
	syscall(SYS_fstat, (int)0, &st);
	asm volatile("" ::: "memory"); // barrier();
}

// https://www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
static int compare_ull(const void *arg1, const void *arg2) {
	unsigned long long ull_a = *(unsigned long long *)arg1;
	unsigned long long ull_b = *(unsigned long long *)arg2;
	return (ull_a > ull_b) - (ull_a < ull_b);
}

int main() {
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	unsigned long long newfstat_samples[N_ITERATIONS];
	unsigned long long newfstatat_samples[N_ITERATIONS];
	unsigned long long total_newfstat_time = 0;
	unsigned long long total_newfstatat_time = 0;
	
	// waza for newfstatat
	int j = 0;
	do {
		int z = 0;
		do {
			newfstatat_call();
			newfstatat_call();
			z = z + 1;
		} while (z < N_BATCH);
		j = j + 1;
		asm volatile("nop");
	} while (j < 250);

	int k = 0;
	do {
		unsigned long long t0 = time_now_ns();
		int z = 0;
		do {
			newfstatat_call();
			z = z + 1;
		} while (z < N_BATCH);	
		unsigned long long t1 = time_now_ns();
		newfstatat_samples[k] = (t1 - t0) / N_BATCH;
		total_newfstatat_time += newfstatat_samples[k];
		k = k + 1;
		asm volatile("nop");
	} while (k < N_ITERATIONS);

	// waza for newfstat
	j = 0;
	do {
		int z = 0;
		do {
			newfstat_call();
			newfstat_call();
			z = z + 1;
		} while (z < N_BATCH);
		j = j + 1;
		asm volatile("nop");
	} while (j < 250);

	int i = 0;
	do {
		int z = 0;
		unsigned long long t0 = time_now_ns();
		do {
			newfstat_call();
			z = z + 1;
		} while (z < N_BATCH);
		unsigned long long t1 = time_now_ns();
		newfstat_samples[i] = (t1 - t0) / N_BATCH;
		total_newfstat_time += newfstat_samples[i];
		i = i + 1;
		asm volatile("nop");
	} while (i < N_ITERATIONS);
	
	long long avg_newfstat_ns = total_newfstat_time / N_ITERATIONS;
	long long avg_newfstatat_ns = total_newfstatat_time / N_ITERATIONS;
	
	double ratio = (double)avg_newfstatat_ns / avg_newfstat_ns;
	double percent_overhead = (ratio - 1.0) * 100.0;
	
	// sort the array we got
	qsort(newfstat_samples, N_ITERATIONS, sizeof(newfstat_samples[0]), compare_ull);
	qsort(newfstatat_samples, N_ITERATIONS, sizeof(newfstatat_samples[0]), compare_ull);
	
	unsigned long long median_newfstat;
	unsigned long long median_newfstatat;
	// grab that shit in the middle
	median_newfstat = newfstat_samples[N_ITERATIONS / 2];
	median_newfstatat = newfstatat_samples[N_ITERATIONS / 2];
	
	double ratio_overhead = (double)median_newfstatat / (double)median_newfstat;
	double median_overhead = (ratio_overhead - 1.0) * 100.0;
	
	printf("iterations: %d\n", N_ITERATIONS);
	printf("average newfstat: %lld ns\n", avg_newfstat_ns);
	printf("average newfstatat: %lld ns\n", avg_newfstatat_ns);
	printf("average newfstatat/newfstat overhead: %.2f%%\n", percent_overhead);
	printf("median newfstat: %llu ns\n", median_newfstat);
	printf("median newfstatat: %llu ns\n", median_newfstatat);
	printf("median newfstatat/newfstat overhead: %.2f%%\n", median_overhead);

	// manual hooks doesnt seem exploitable for reals
#if 0
	if (median_overhead >= 100000) {
		if (percent_overhead >= 10000)
			printf("confidence: high\n");
		else if (percent_overhead > 100)
			printf("confidence: maybe\n");
		else
			printf("verdict: try again\n");
	} else
		printf("verdict: try again\n");
#endif
	
	return 0;
}
