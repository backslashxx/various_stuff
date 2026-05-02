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
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "small_rt.h"

// zig cc -target aarch64-linux -s -Oz -Wno-int-conversion -std=c23 -static -Wl,--gc-sections,-z,norelro -fno-unwind-tables -Wl,--entry=__start -flto -fmerge-all-constants -Wno-unknown-attributes syscall_bench_nocrt0/bench.c -o bench_arm64
// zig cc -target arm-linux -s -Oz -Wno-int-conversion -std=c23 -static -Wl,--gc-sections,-z,norelro -fno-unwind-tables -Wl,--entry=__start -flto -fmerge-all-constants -Wno-unknown-attributes syscall_bench_nocrt0/bench.c -o bench_arm

#define N_ITERATIONS 1000000
#define N_ITERATION_DIGITS 7

__attribute__((noinline))
static void print_out(const char *buf, unsigned long len)
{
	__syscall(SYS_write, 1, (long)buf, len, NONE, NONE, NONE);
}

/*
 *	long_to_str, long to string
 *	
 *	converts an int to string with expected len
 *	
 *	caller is reposnible for sanity!
 *	no bounds check, no nothing, do not pass len = 0
 *	
 *	example:
 *	long_to_str(10123, 5, buf); // where buf is char buf[5]; atleast
 */
__attribute__((noinline))
static void long_to_str(unsigned long number, unsigned long len, char *buf)
{

start:
	buf[len - 1] = 48 + (number % 10);
	number = number / 10;
	len--;

	if (len > 0)
		goto start;

	return;
}

__attribute__((always_inline))
static bool check_seccomp() {
	pid_t pid = __syscall(SYS_clone, SIGCHLD, NULL, NULL, NULL, NULL, NULL);
	if (pid == -1)
		return false;

	if (pid == 0) {
		__syscall(SYS_swapoff, NULL, NULL, NULL, NULL, NULL, NULL);
		__syscall(SYS_exit, 0, NULL, NULL, NULL, NULL, NULL);
		__builtin_unreachable();
	}

	int status = 0;
	__syscall(SYS_wait4, pid, &status, 0, NULL, NULL, NULL);
	if (WIFSIGNALED(status))
		return true; // means it died weirdly
	
	return false;
}

/**
 * NOTE: this might be actually slower now as this forces a syscall
 * clock_gettime by default is routed through vDSO.
 * but I think this is fair game as we are benchmarking syscalls
 */
 
#if defined(__arm__) 

#define SYS_newfstatat SYS_fstatat64

// armeabi syscall 263 SYS_clock_gettime got renamed to SYS_clock_gettime32
// https://syscalls.mebeim.net/?table=arm/32/eabi/v5.0
// https://syscalls.mebeim.net/?table=arm/32/eabi/v6.17
// NOTE: use is discouraged for due to y2038, but it should not matter for benchmarking

// https://elixir.bootlin.com/linux/v7.0.1/source/include/vdso/time32.h
struct old_timespec32 {
	int32_t	tv_sec;
	int32_t	tv_nsec;
};

__attribute__((hot, always_inline))
static unsigned long long time_now_ns() {
	struct old_timespec32 ts32;

	// CLOCK_MONOTONIC is 1
	__syscall(SYS_clock_gettime32, 1, (long)&ts32, NONE, NONE, NONE, NONE);

	return (unsigned long long)ts32.tv_sec * 1000000000ULL + ts32.tv_nsec;
}

#else /* ! arm */

__attribute__((hot, always_inline))
static unsigned long long time_now_ns() {
	struct timespec ts;
	long clk_id;

#ifdef CLOCK_MONOTONIC_RAW
	clk_id = CLOCK_MONOTONIC_RAW;
#else
	clk_id = CLOCK_MONOTONIC;
#endif
	__syscall(SYS_clock_gettime, clk_id, (long)&ts, NONE, NONE, NONE, NONE);
	return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif // __arm__

__attribute__((always_inline))
static int c_main(long argc, char **argv, char **envp)
{
	bool is_seccomp_enabled = check_seccomp();

	// Pin to core 0 for consistency
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	__syscall(SYS_sched_setaffinity, 0, sizeof(cpuset), &cpuset, NULL, NULL, NULL);
	__syscall(SYS_setpriority, 0, 0, -20, NONE, NONE, NONE);

	uint_fast64_t t0, t1;
	uint_fast32_t i;
	struct stat st;

	const char run_template[] = "[+] run ";
	char iter_buf[N_ITERATION_DIGITS];
	const char iter_template[] = " iterations per syscall...\n";

	print_out(run_template, sizeof(run_template) - 1);
	long_to_str(N_ITERATIONS, N_ITERATION_DIGITS, iter_buf);
	print_out(iter_buf, N_ITERATION_DIGITS);
	print_out(iter_template, sizeof(iter_template) - 1);

	const char su_found[] = "[+] /system/bin/su found! sucompat is active.\n";
	const char su_not_found[] = "[-] /system/bin/su not found! sucompat is disabled.\n";
	const char seccomp_enabled[] = "[+] seccomp enabled\n";
	const char seccomp_disabled[] = "[-] seccomp disabled\n";
	const char extra_lines[] = 
		"[!] note:\n"
		"[1] NULL\n"
		"[2] /dev/null\n"
		"[3] /system/bin/su_\n"
		"[*] Lower is better\n";

	if (!__syscall(SYS_faccessat, AT_FDCWD, (long)"/system/bin/su", F_OK, NONE, NONE, NONE))
        	print_out(su_found, sizeof(su_found) - 1 );
        else
        	print_out(su_not_found, sizeof(su_not_found) - 1 );

	if (is_seccomp_enabled)
        	print_out(seccomp_enabled, sizeof(seccomp_enabled) - 1 );
	else
        	print_out(seccomp_disabled, sizeof(seccomp_disabled) - 1 );

	print_out(extra_lines, sizeof(extra_lines) - 1 );

	char newfstatat_template[] = "[1] newfstatat:\t ";
	char faccessat_template[] = "[1] faccessat:\t ";
	char execve_template[] = "[1] execve:\t ";

	char result_template[] = "(0000000 ns avg)\n";
	char newline[] = "\n";

	print_out(newline, sizeof(newline) - 1 );
	i = 0;
	t0 = time_now_ns();
bench_newfstatat:
	__syscall(SYS_newfstatat, AT_FDCWD, NULL, &st, 0, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat;

	t1 = time_now_ns();
	print_out(newfstatat_template, sizeof(newfstatat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_faccessat:
	__syscall(SYS_faccessat, AT_FDCWD, NULL, F_OK, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat;

	t1 = time_now_ns();
	print_out(faccessat_template, sizeof(faccessat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_execve:
	__syscall(SYS_execve, NULL, NULL, NULL, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve;

	t1 = time_now_ns();
	print_out(execve_template, sizeof(execve_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	newfstatat_template[1] = '2';
	faccessat_template[1] = '2';
	execve_template[1] = '2';
	print_out(newline, sizeof(newline) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_newfstatat_with_null:
	__syscall(SYS_newfstatat, AT_FDCWD, "/dev/null", &st, 0, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat_with_null;

	t1 = time_now_ns();
	print_out(newfstatat_template, sizeof(newfstatat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_faccessat_with_null:
	__syscall(SYS_faccessat, AT_FDCWD, "/dev/null", F_OK, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat_with_null;

	t1 = time_now_ns();
	print_out(faccessat_template, sizeof(faccessat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_execve_with_null:
	__syscall(SYS_execve, "/dev/null", NULL, NULL, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve_with_null;

	t1 = time_now_ns();
	print_out(execve_template, sizeof(execve_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	newfstatat_template[1] = '3';
	faccessat_template[1] = '3';
	execve_template[1] = '3';
	print_out(newline, sizeof(newline) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_newfstatat_with_near_miss:
	__syscall(SYS_newfstatat, AT_FDCWD, "/system/bin/su_", &st, 0, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_newfstatat_with_near_miss;

	t1 = time_now_ns();
	print_out(newfstatat_template, sizeof(newfstatat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_faccessat_with_near_miss:
	__syscall(SYS_faccessat, AT_FDCWD, "/system/bin/su_", F_OK, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_faccessat_with_near_miss;

	t1 = time_now_ns();
	print_out(faccessat_template, sizeof(faccessat_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	i = 0;
	t0 = time_now_ns();
bench_execve_with_near_miss:
	__syscall(SYS_execve, "/system/bin/su_", NULL, NULL, NULL, NULL, NULL);
	i++;
	if (i < N_ITERATIONS)
		goto bench_execve_with_near_miss;

	t1 = time_now_ns();
	print_out(execve_template, sizeof(execve_template) - 1 );
	long_to_str((t1 - t0) / N_ITERATIONS, 7, result_template + 1);
	print_out(result_template, sizeof(result_template) - 1 );

	return 0;
}

__attribute__((used))
void prep_main(long *sp)
{
	long argc = *sp;
	char **argv = (char **)(sp + 1);
	char **envp = argv + argc + 1; // we need to offset it by the number of argc's!

	long exit_code = c_main(argc, argv, envp);
	__syscall(SYS_exit, exit_code, NONE, NONE, NONE, NONE, NONE);
	__builtin_unreachable();
}
