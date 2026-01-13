#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Force inline - will be inlined at -O2
static inline uint64_t fast_hash(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

// Never inline - always appears in call stack
__attribute__((noinline))
uint64_t calculate_checksum(uint64_t *data, size_t len) {
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        // fast_hash will be inlined here
        sum += fast_hash(data[i]);
    }
    return sum;
}

// Intermediate function
__attribute__((noinline))
uint64_t process_data(uint64_t *data, size_t len, int iterations) {
    uint64_t result = 0;
    for (int i = 0; i < iterations; i++) {
        result ^= calculate_checksum(data, len);
    }
    return result;
}

int main(int argc, char **argv) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    const size_t DATA_SIZE = 10000;
    const int ITERATIONS = 50000;
    
    uint64_t *data = malloc(DATA_SIZE * sizeof(uint64_t));
    if (!data) {
        perror("malloc");
        return 1;
    }
    
    // Initialize with random data
    for (size_t i = 0; i < DATA_SIZE; i++) {
        data[i] = (uint64_t)rand() << 32 | rand();
    }
    
    printf("Starting CPU-intensive workload...\n");
    printf("Data size: %zu elements, Iterations: %d\n", DATA_SIZE, ITERATIONS);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    uint64_t result = process_data(data, DATA_SIZE, ITERATIONS);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Result: 0x%016lx\n", result);
    printf("Elapsed time: %.3f seconds\n", elapsed);
    
    free(data);
    return 0;
}
