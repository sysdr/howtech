// allocator_benchmark.c - Compare allocation patterns
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NUM_ALLOCS 100000
#define OBJECT_SIZE 512

static inline unsigned long long rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

void benchmark_malloc(void) {
    void **ptrs = malloc(NUM_ALLOCS * sizeof(void*));
    unsigned long long start, end;
    
    start = rdtsc();
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(OBJECT_SIZE);
        if (!ptrs[i]) {
            perror("malloc failed");
            exit(1);
        }
        memset(ptrs[i], 0, OBJECT_SIZE);
    }
    end = rdtsc();
    
    printf("malloc allocations: %llu cycles\n", end - start);
    printf("Average per alloc: %llu cycles\n", (end - start) / NUM_ALLOCS);
    
    start = rdtsc();
    for (int i = 0; i < NUM_ALLOCS; i++) {
        free(ptrs[i]);
    }
    end = rdtsc();
    
    printf("malloc frees: %llu cycles\n", end - start);
    printf("Average per free: %llu cycles\n\n", (end - start) / NUM_ALLOCS);
    
    free(ptrs);
}

void show_slabinfo(void) {
    FILE *f = fopen("/proc/slabinfo", "r");
    if (!f) {
        return;  // Not accessible without sudo
    }
    
    char line[512];
    printf("\n=== Relevant /proc/slabinfo entries ===\n");
    printf("%-20s %10s %10s %10s %10s\n", 
           "Cache", "Active", "Total", "ObjSize", "PerSlab");
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "kmalloc-512") || 
            strstr(line, "task_struct") ||
            strstr(line, "dentry")) {
            char name[64];
            unsigned long active, total, objsize, objperslab;
            if (sscanf(line, "%s %lu %lu %lu %*u %*u : tunables %*d %*d %*d : slabdata %*u %*u %*u", 
                      name, &active, &total, &objsize) >= 4) {
                // Simple approximation
                objperslab = 4096 / objsize;
                printf("%-20s %10lu %10lu %10lu %10lu\n", 
                       name, active, total, objsize, objperslab);
            }
        }
    }
    fclose(f);
}

int main(void) {
    printf("=== Userspace Allocator Benchmark ===\n\n");
    printf("Comparing allocation patterns with %d x %d byte objects\n\n", 
           NUM_ALLOCS, OBJECT_SIZE);
    
    benchmark_malloc();
    show_slabinfo();
    
    printf("\nKernel slab caches use similar techniques:\n");
    printf("- Per-CPU freelists (SLUB)\n");
    printf("- Object reuse without returning to page allocator\n");
    printf("- Cache-line alignment for better CPU cache performance\n");
    printf("\nRun './monitor.sh' to see real-time slab statistics\n");
    
    return 0;
}
