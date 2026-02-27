/*
 * latency_compare.c — Syscall latency on safety-relevant operations
 *
 * On a safety RTOS (R5 side), interrupt response is deterministic.
 * On Linux (A-cluster), even simple operations have jitter due to preemption,
 * cache misses, and scheduler interference. This demonstrates the jitter
 * profile that makes Linux unsuitable as the ASIL B execution environment.
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -o latency_compare latency_compare.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>

#define SAMPLES         1024
#define NS_PER_US       1000LL
#define NS_PER_MS       1000000LL

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void compute_stats(uint64_t *samples, int n,
                           uint64_t *out_min, uint64_t *out_max,
                           uint64_t *out_avg, uint64_t *out_p99) {
    uint64_t sum = 0, mn = UINT64_MAX, mx = 0;
    for (int i = 0; i < n; i++) {
        if (samples[i] < mn) mn = samples[i];
        if (samples[i] > mx) mx = samples[i];
        sum += samples[i];
    }
    *out_min = mn;
    *out_max = mx;
    *out_avg = sum / (uint64_t)n;

    /* Simple p99: sort copy */
    uint64_t *sorted = malloc((size_t)n * sizeof(uint64_t));
    if (!sorted) { *out_p99 = mx; return; }
    memcpy(sorted, samples, (size_t)n * sizeof(uint64_t));
    /* Insertion sort — fine for 1024 samples */
    for (int i = 1; i < n; i++) {
        uint64_t key = sorted[i]; int j = i - 1;
        while (j >= 0 && sorted[j] > key) { sorted[j+1] = sorted[j]; j--; }
        sorted[j+1] = key;
    }
    *out_p99 = sorted[(int)(n * 99 / 100)];
    free(sorted);
}

static void print_histogram(uint64_t *samples, int n, const char *label) {
    uint64_t mn, mx, avg, p99;
    compute_stats(samples, n, &mn, &mx, &avg, &p99);

    printf("  \033[1m%s\033[0m\n", label);
    printf("  %-8s %6llu ns\n", "min:", (unsigned long long)mn);
    printf("  %-8s %6llu ns\n", "avg:", (unsigned long long)avg);
    printf("  %-8s %6llu ns\n", "p99:", (unsigned long long)p99);
    printf("  %-8s %6llu ns\n", "max:", (unsigned long long)mx);

    /* ASCII histogram over 8 buckets */
    uint64_t range = (mx - mn) + 1;
    uint64_t bw = range / 8 + 1;
    int buckets[8] = {0};
    for (int i = 0; i < n; i++) {
        int b = (int)((samples[i] - mn) / bw);
        if (b >= 8) b = 7;
        buckets[b]++;
    }
    int bmax = 1;
    for (int i = 0; i < 8; i++) if (buckets[i] > bmax) bmax = buckets[i];
    printf("\n  Distribution (%d samples):\n", n);
    for (int i = 0; i < 8; i++) {
        int bar = (buckets[i] * 30) / bmax;
        printf("  %5llu–%-5llu │", (unsigned long long)(mn + (uint64_t)i * bw),
               (unsigned long long)(mn + (uint64_t)(i+1) * bw - 1));
        for (int j = 0; j < bar; j++) printf("█");
        printf(" %d\n", buckets[i]);
    }
    printf("\n");
}

/* Measure clock_gettime latency as a proxy for minimal kernel round-trip */
static void measure_syscall_jitter(void) {
    uint64_t *samples = malloc(SAMPLES * sizeof(uint64_t));
    if (!samples) { perror("malloc"); return; }

    /* Warm up */
    for (int i = 0; i < 64; i++) { volatile uint64_t t = now_ns(); (void)t; }

    for (int i = 0; i < SAMPLES; i++) {
        uint64_t t0 = now_ns();
        uint64_t t1 = now_ns();
        samples[i] = t1 - t0;
    }
    print_histogram(samples, SAMPLES,
        "clock_gettime(MONOTONIC_RAW) round-trip [Linux VDSO path]");
    free(samples);
}

/* Measure pipe read/write latency — simulates UIO IRQ notification path */
static void measure_pipe_irq_latency(void) {
    int pfd[2];
    if (pipe(pfd) == -1) { perror("pipe"); return; }

    uint64_t *samples = malloc(SAMPLES * sizeof(uint64_t));
    if (!samples) { close(pfd[0]); close(pfd[1]); return; }

    uint32_t val = 1;
    /* Warm up */
    for (int i = 0; i < 32; i++) {
        if (write(pfd[1], &val, 4) < 0) break;
        if (read(pfd[0], &val, 4) < 0) break;
    }

    for (int i = 0; i < SAMPLES; i++) {
        uint64_t t0 = now_ns();
        if (write(pfd[1], &val, 4) < 0) { samples[i] = 0; continue; }
        if (read(pfd[0], &val, 4) < 0) { samples[i] = 0; continue; }
        uint64_t t1 = now_ns();
        samples[i] = t1 - t0;
    }

    print_histogram(samples, SAMPLES,
        "pipe write+read [UIO interrupt notification proxy]");

    close(pfd[0]);
    close(pfd[1]);
    free(samples);
}

static void print_asil_context(void) {
    printf("\033[1;33m  Why jitter disqualifies Linux from ASIL B safety paths:\033[0m\n\n");
    printf("  ISO 26262 Part 6 requires bounded worst-case execution time (WCET)\n");
    printf("  analysis for safety functions. Linux scheduler jitter, cache\n");
    printf("  thrashing from non-safety workloads, and interrupt coalescing\n");
    printf("  make WCET analysis for Linux effectively impossible.\n\n");
    printf("  The Orin R5 safety cluster with lock-step cores provides:\n");
    printf("  • Deterministic interrupt response (no scheduler interference)\n");
    printf("  • MPU-enforced memory regions (no MMU TLB shootdowns)\n");
    printf("  • Hardware error correction (ECC on TCM, cache parity)\n");
    printf("  • Bounded context switch time (RTOS, no CFS)\n\n");
    printf("  The Linux A-cluster p99/max latency you see above is why\n");
    printf("  the safety function stays on R5 — not in a kernel module.\n\n");
}

int main(void) {
    /* Pin to a CPU to reduce noise */
    cpu_set_t cs;
    CPU_ZERO(&cs); CPU_SET(0, &cs);
    sched_setaffinity(0, sizeof(cs), &cs);

    /* Lock pages to avoid page fault jitter */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    printf("\n\033[1;34m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;34m║\033[0m  Latency Jitter Analysis                             \033[1;34m║\033[0m\n");
    printf("\033[1;34m║\033[0m  Why Linux cannot host ASIL B safety functions        \033[1;34m║\033[0m\n");
    printf("\033[1;34m╚══════════════════════════════════════════════════════╝\033[0m\n\n");

    measure_syscall_jitter();
    measure_pipe_irq_latency();
    print_asil_context();

    munlockall();
    return EXIT_SUCCESS;
}
