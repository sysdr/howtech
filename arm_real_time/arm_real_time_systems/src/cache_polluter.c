#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>

#define CACHE_SIZE (1024 * 1024)  // 1MB to pollute L2
#define ITERATIONS 1000000

int main(int argc, char **argv) {
    int cpu = 1; // Default to CPU 1
    if (argc > 1) cpu = atoi(argv[1]);
    
    // Pin to specified CPU
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity");
        return 1;
    }
    
    printf("Cache polluter running on CPU %d\n", cpu);
    
    // Allocate cache pollution buffer
    char *buffer = malloc(CACHE_SIZE);
    if (!buffer) {
        perror("malloc");
        return 1;
    }
    
    // Continuously pollute cache
    volatile uint64_t sum = 0;
    while (1) {
        for (int i = 0; i < ITERATIONS; i++) {
            // Random memory access pattern to maximize cache pollution
            int offset = (rand() % (CACHE_SIZE - 64));
            sum += buffer[offset];
            buffer[offset] = (char)(sum & 0xFF);
        }
    }
    
    free(buffer);
    return 0;
}
