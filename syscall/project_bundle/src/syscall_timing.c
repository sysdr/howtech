#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define ITERATIONS 1000000
#define WARMUP 10000

// Get high-resolution timestamp using RDTSC
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );
    return ((uint64_t)hi << 32) | lo;
}

// Measure syscall latency using getpid()
double measure_syscall_latency(void) {
    uint64_t start, end, total = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP; i++) {
        getpid();
    }
    
    // Actual measurement
    for (int i = 0; i < ITERATIONS; i++) {
        start = rdtsc();
        getpid();
        end = rdtsc();
        total += (end - start);
    }
    
    return (double)total / ITERATIONS;
}

// Measure vDSO-optimized call (clock_gettime with CLOCK_MONOTONIC)
double measure_vdso_latency(void) {
    struct timespec ts;
    uint64_t start, end, total = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP; i++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    
    // Actual measurement
    for (int i = 0; i < ITERATIONS; i++) {
        start = rdtsc();
        clock_gettime(CLOCK_MONOTONIC, &ts);
        end = rdtsc();
        total += (end - start);
    }
    
    return (double)total / ITERATIONS;
}

// Direct syscall using inline assembly
static inline long direct_syscall(long number, long arg1) {
    long ret;
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = arg1;
    
    __asm__ __volatile__ (
        "syscall"
        : "=a"(ret)
        : "0"(rax), "r"(rdi)
        : "rcx", "r11", "memory"
    );
    
    return ret;
}

// Measure direct syscall (bypassing libc)
double measure_direct_syscall_latency(void) {
    uint64_t start, end, total = 0;
    
    // Warmup
    for (int i = 0; i < WARMUP; i++) {
        direct_syscall(SYS_getpid, 0);
    }
    
    // Actual measurement
    for (int i = 0; i < ITERATIONS; i++) {
        start = rdtsc();
        direct_syscall(SYS_getpid, 0);
        end = rdtsc();
        total += (end - start);
    }
    
    return (double)total / ITERATIONS;
}

// Get CPU frequency to convert cycles to nanoseconds
double get_cpu_freq_mhz(void) {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        fprintf(stderr, "Warning: Cannot read CPU frequency, using default 3000 MHz\n");
        return 3000.0;
    }
    
    char line[256];
    double freq = 3000.0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "cpu MHz", 7) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                freq = atof(colon + 1);
                break;
            }
        }
    }
    fclose(fp);
    return freq;
}

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║          SYSCALL INSTRUCTION LATENCY MEASUREMENT             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");
    
    double cpu_freq = get_cpu_freq_mhz();
    printf("CPU Frequency: %.2f MHz\n", cpu_freq);
    printf("Measurement iterations: %d\n\n", ITERATIONS);
    
    // Measure different syscall types
    printf("Measuring syscall latencies...\n\n");
    
    double cycles_libc = measure_syscall_latency();
    double cycles_direct = measure_direct_syscall_latency();
    double cycles_vdso = measure_vdso_latency();
    
    // Convert cycles to nanoseconds
    double ns_per_cycle = 1000.0 / cpu_freq;
    double ns_libc = cycles_libc * ns_per_cycle;
    double ns_direct = cycles_direct * ns_per_cycle;
    double ns_vdso = cycles_vdso * ns_per_cycle;
    
    printf("┌─────────────────────────────────────────┬─────────┬──────────┐\n");
    printf("│ Syscall Type                            │ Cycles  │ Latency  │\n");
    printf("├─────────────────────────────────────────┼─────────┼──────────┤\n");
    printf("│ getpid() via libc                       │ %7.0f │ %6.0f ns │\n", cycles_libc, ns_libc);
    printf("│ getpid() via direct syscall             │ %7.0f │ %6.0f ns │\n", cycles_direct, ns_direct);
    printf("│ clock_gettime() via vDSO                │ %7.0f │ %6.0f ns │\n", cycles_vdso, ns_vdso);
    printf("└─────────────────────────────────────────┴─────────┴──────────┘\n\n");
    
    // Calculate overhead
    printf("Analysis:\n");
    printf("  • Syscall overhead: ~%.0f ns (%.0f CPU cycles)\n", ns_direct, cycles_direct);
    printf("  • vDSO speedup: %.1fx faster than real syscall\n", ns_direct / ns_vdso);
    printf("  • At 1M syscalls/sec: %.1f ms CPU time spent in transitions\n", ns_direct / 1000.0);
    
    if (ns_direct > 250) {
        printf("  • KPTI is likely enabled (syscall > 250ns)\n");
        printf("  • Without KPTI, expect ~150ns latency\n");
    } else {
        printf("  • KPTI may be disabled (syscall < 250ns)\n");
    }
    
    printf("\n");
    
    return 0;
}
