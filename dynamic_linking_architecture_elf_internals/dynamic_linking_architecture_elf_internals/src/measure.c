#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

extern void example_function(const char *msg);

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

#define WARMUP_CALLS 10
#define MEASURE_CALLS 1000

int main(void) {
    uint64_t cycles[MEASURE_CALLS];
    uint64_t start, end;
    
    printf("=== Precise PLT Call Timing ===\n\n");
    
    // First call (resolver overhead)
    printf("[First Call - With Resolver]\n");
    start = rdtsc();
    example_function("resolver call");
    end = rdtsc();
    printf("Cycles: %lu (~%.2f ns)\n\n", end - start, (double)(end - start) / 2.5);
    
    // Warmup
    for (int i = 0; i < WARMUP_CALLS; i++) {
        example_function("warmup");
    }
    
    // Measure subsequent calls
    printf("[Subsequent Calls - %d iterations]\n", MEASURE_CALLS);
    for (int i = 0; i < MEASURE_CALLS; i++) {
        start = rdtsc();
        example_function("measure");
        end = rdtsc();
        cycles[i] = end - start;
    }
    
    // Calculate statistics
    uint64_t sum = 0, min = UINT64_MAX, max = 0;
    for (int i = 0; i < MEASURE_CALLS; i++) {
        sum += cycles[i];
        if (cycles[i] < min) min = cycles[i];
        if (cycles[i] > max) max = cycles[i];
    }
    uint64_t avg = sum / MEASURE_CALLS;
    
    printf("Average: %lu cycles (~%.2f ns)\n", avg, (double)avg / 2.5);
    printf("Min: %lu cycles (~%.2f ns)\n", min, (double)min / 2.5);
    printf("Max: %lu cycles (~%.2f ns)\n", max, (double)max / 2.5);
    
    // Histogram
    printf("\nCall overhead distribution:\n");
    int buckets[10] = {0};
    for (int i = 0; i < MEASURE_CALLS; i++) {
        int bucket = (cycles[i] - min) * 9 / (max - min);
        if (bucket > 9) bucket = 9;
        buckets[bucket]++;
    }
    
    for (int i = 0; i < 10; i++) {
        printf("[%4lu-%4lu]: ", min + (max-min)*i/10, min + (max-min)*(i+1)/10);
        for (int j = 0; j < buckets[i] / 10; j++) printf("█");
        printf(" %d\n", buckets[i]);
    }
    
    return 0;
}
