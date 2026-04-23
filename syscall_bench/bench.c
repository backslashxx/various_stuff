#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <sched.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

// zig cc -target aarch64-linux -Oz -s -static bench.c -o bench -Wno-format
// taskset -c 0 ./bench

#define N_ITERATIONS 1000000

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

int main() {
	// Pin to core 0 for consistency
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	uint_fast64_t t0, t1;
	uint_fast32_t i;
	struct stat st;

	printf("[+] run %d iterations per syscall...\n", N_ITERATIONS);

	if (!syscall(SYS_newfstatat, AT_FDCWD, "/system/bin/su", &st, 0))
        	printf("[+] /system/bin/su found! sucompat is active.\n");
        else
        	printf("[+] /system/bin/su not found! sucompat is disabled.\n");       	

	i = 0;
	t0 = time_now_ns();
bench_newfstatat:
	syscall(SYS_newfstatat, AT_FDCWD, (int)0, &st, 0);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat;

	t1 = time_now_ns();
	printf("newfstatat: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_faccessat:
	syscall(SYS_faccessat, AT_FDCWD, "", F_OK);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat;

	t1 = time_now_ns();
	printf("faccessat: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_execve:
	syscall(SYS_execve, NULL, NULL, NULL);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve;

	t1 = time_now_ns();
	printf("execve:	 %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_newfstatat_with_null:
	syscall(SYS_newfstatat, AT_FDCWD, "/dev/null", &st, 0);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat_with_null;

	t1 = time_now_ns();
	printf("newfstatat w/ /dev/null %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_faccessat_with_null:
	syscall(SYS_faccessat, AT_FDCWD, "/dev/null", F_OK);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat_with_null;

	t1 = time_now_ns();
	printf("faccessat w/ /dev/null %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_execve_with_null:
	syscall(SYS_execve, "/dev/null", NULL, NULL);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve_with_null;

	t1 = time_now_ns();
	printf("execve w/ /dev/null: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_newfstatat_with_near_miss:
	syscall(SYS_newfstatat, AT_FDCWD, "/system/bin/ss_", &st, 0);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat_with_near_miss;

	t1 = time_now_ns();
	printf("newfstatat w/ near miss: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_faccessat_with_near_miss:
	syscall(SYS_faccessat, AT_FDCWD, "/system/bin/ss_", F_OK);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat_with_near_miss;

	t1 = time_now_ns();
	printf("faccessat w/ near miss: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	i = 0;
	t0 = time_now_ns();
bench_execve_with_near_miss:
	syscall(SYS_execve, "/system/bin/ss_", NULL, NULL);
	asm volatile("" ::: "memory");
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve_with_near_miss;

	t1 = time_now_ns();
	printf("execve w/ near miss: %llu ns total (%llu ns avg)\n", (t1 - t0), (t1 - t0) / N_ITERATIONS);

	return 0;
}
