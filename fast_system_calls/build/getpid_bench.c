#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>

#define ITERATIONS 10000000
#define NS_PER_SEC 1000000000L

static inline long long timespec_diff_ns(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * NS_PER_SEC + 
           (end->tv_nsec - start->tv_nsec);
}

// Direct syscall - bypasses glibc and vDSO completely
static inline pid_t raw_getpid(void) {
    return (pid_t)syscall(SYS_getpid);
}

int main(int argc, char **argv) {
    struct timespec start, end;
    long long elapsed_ns;
    double ns_per_call;
    int use_vdso = 1;
    
    if (argc > 1 && strcmp(argv[1], "--no-vdso") == 0) {
        use_vdso = 0;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         getpid() Performance Benchmark                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Configuration:\n");
    printf("  • Iterations: %d\n", ITERATIONS);
    printf("  • Method: %s\n", use_vdso ? "glibc getpid() with vDSO" : "raw syscall(SYS_getpid)");
    printf("\n");
    
    // Warmup
    for (int i = 0; i < 1000; i++) {
        if (use_vdso) {
            getpid();
        } else {
            raw_getpid();
        }
    }
    
    printf("Running benchmark");
    fflush(stdout);
    
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime start");
        return 1;
    }
    
    if (use_vdso) {
        // Use glibc's getpid() - will use vDSO if available
        for (int i = 0; i < ITERATIONS; i++) {
            getpid();
            if (i % (ITERATIONS / 10) == 0) {
                printf(".");
                fflush(stdout);
            }
        }
    } else {
        // Direct syscall - always goes to kernel
        for (int i = 0; i < ITERATIONS; i++) {
            raw_getpid();
            if (i % (ITERATIONS / 10) == 0) {
                printf(".");
                fflush(stdout);
            }
        }
    }
    
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime end");
        return 1;
    }
    
    elapsed_ns = timespec_diff_ns(&start, &end);
    ns_per_call = (double)elapsed_ns / ITERATIONS;
    
    printf(" Done!\n\n");
    
    printf("Results:\n");
    printf("  • Total time:    %.3f ms\n", elapsed_ns / 1000000.0);
    printf("  • Per call:      %.2f ns\n", ns_per_call);
    printf("  • Throughput:    %.2f M calls/sec\n", ITERATIONS / (elapsed_ns / 1000.0));
    printf("\n");
    
    if (use_vdso && ns_per_call < 20) {
        printf("✓ vDSO is working! Excellent performance (<20ns per call)\n");
    } else if (use_vdso && ns_per_call < 50) {
        printf("⚠ vDSO likely active but some overhead detected\n");
    } else if (!use_vdso) {
        printf("⚠ Direct syscall path (expected to be slower)\n");
    } else {
        printf("✗ vDSO may be disabled! Performance is unusually slow\n");
    }
    printf("\n");
    
    return 0;
}
