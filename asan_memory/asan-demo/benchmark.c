#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 1000000

// Read timestamp counter
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

int main() {
    printf("=== ASAN Performance Benchmark ===\n");
    printf("Iterations: %d malloc/free cycles\n\n", ITERATIONS);
    
    uint64_t start_cycles = rdtsc();
    clock_t start_time = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int *ptr = malloc(100);
        ptr[0] = i;
        ptr[99] = i * 2;
        free(ptr);
    }
    
    clock_t end_time = clock();
    uint64_t end_cycles = rdtsc();
    
    double seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    uint64_t cycles = end_cycles - start_cycles;
    
    printf("Results:\n");
    printf("  Time: %.3f seconds\n", seconds);
    printf("  CPU Cycles: %lu\n", cycles);
    printf("  Cycles per iteration: %lu\n", cycles / ITERATIONS);
    printf("  Operations per second: %.0f\n", ITERATIONS / seconds);
    
#ifdef __SANITIZE_ADDRESS__
    printf("\n  *** Running with ASAN (expect 2-3x overhead) ***\n");
#else
    printf("\n  Running WITHOUT ASAN (baseline performance)\n");
#endif
    
    return 0;
}
