#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>

// Declare external functions from libexample.so
extern void example_function(const char *msg);
extern void another_function(int value);
extern void complex_function(void);

// High-resolution timing using RDTSC
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

// Measure function call overhead
void measure_call_overhead(void) {
    uint64_t start, end, overhead;
    const int iterations = 3;
    
    printf("\n=== Measuring PLT/GOT Call Overhead ===\n");
    
    // First call - will go through resolver (if lazy binding)
    printf("\n[First Call - Lazy Binding Resolver]\n");
    start = rdtsc();
    example_function("First call (resolver overhead)");
    end = rdtsc();
    overhead = end - start;
    printf("Cycles: %lu (~%.2f ns at 2.5GHz)\n", overhead, (double)overhead / 2.5);
    
    // Subsequent calls - direct jump through GOT
    printf("\n[Subsequent Calls - Direct Jump]\n");
    for (int i = 0; i < iterations; i++) {
        start = rdtsc();
        example_function("Subsequent call (no resolver)");
        end = rdtsc();
        overhead = end - start;
        printf("Cycles: %lu (~%.2f ns at 2.5GHz)\n", overhead, (double)overhead / 2.5);
    }
    
    // Test other functions
    printf("\n[Other Functions]\n");
    start = rdtsc();
    another_function(42);
    end = rdtsc();
    printf("another_function cycles: %lu\n", end - start);
    
    start = rdtsc();
    complex_function();
    end = rdtsc();
    printf("complex_function cycles: %lu\n", end - start);
}

// Show GOT contents using dladdr
void show_got_info(void) {
    Dl_info info;
    
    printf("\n=== Dynamic Linking Information ===\n");
    
    if (dladdr((void*)example_function, &info)) {
        printf("example_function:\n");
        printf("  Symbol: %s\n", info.dli_sname ? info.dli_sname : "unknown");
        printf("  Shared object: %s\n", info.dli_fname);
        printf("  Address: %p\n", info.dli_saddr);
    }
    
    if (dladdr((void*)printf, &info)) {
        printf("\nprintf:\n");
        printf("  Symbol: %s\n", info.dli_sname ? info.dli_sname : "unknown");
        printf("  Shared object: %s\n", info.dli_fname);
        printf("  Address: %p\n", info.dli_saddr);
    }
}

int main(int argc, char **argv) {
    (void)argc; // Suppress unused parameter warning
    printf("=== GOT/PLT Dynamic Linking Demo ===\n");
    printf("PID: %d\n", getpid());
    printf("Binary: %s\n\n", argv[0]);
    
    // Show memory maps
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/maps | grep -E '(\\[heap\\]|\\[stack\\]|%s|libexample)'", 
             getpid(), argv[0]);
    printf("=== Memory Layout ===\n");
    int ret = system(cmd);
    (void)ret; // Suppress unused return value warning
    
    // Measure call overhead
    measure_call_overhead();
    
    // Show GOT information
    show_got_info();
    
    printf("\n=== Demo Complete ===\n");
    printf("Run with ltrace to see resolver calls:\n");
    printf("  ltrace -C ./output/test-lazy\n");
    printf("  ltrace -C ./output/test-eager\n");
    
    return 0;
}
