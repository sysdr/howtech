#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>

// RDTSC for high-precision timing
static inline uint64_t rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Function that calls many library functions
void call_many_functions(void) {
    // String functions
    char buf[256];
    snprintf(buf, sizeof(buf), "Test string %d", 42);
    (void)strlen(buf);
    strcpy(buf, "Another string");
    strcat(buf, " appended");
    (void)strcmp(buf, "test");
    
    // Math functions
    double x = sin(3.14159);
    x = cos(x);
    x = sqrt(fabs(x));
    x = pow(x, 2.0);
    x = log(x + 1.0);
    x = exp(x);
    
    // Memory functions
    void *ptr1 = malloc(1024);
    void *ptr2 = calloc(100, 10);
    void *ptr3 = realloc(ptr2, 2048);
    free(ptr1);
    free(ptr3);
    
    // File operations
    FILE *f = tmpfile();
    if (f) {
        fprintf(f, "test data");
        fseek(f, 0, SEEK_SET);
        char read_buf[64];
        char *result = fgets(read_buf, sizeof(read_buf), f);
        (void)result;  // Suppress unused variable warning
        fclose(f);
    }
    
    // Time functions
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // More string operations
    (void)strtok(buf, " ");
    int val1 = atoi("12345");
    double val2 = atof("3.14159");
    (void)val1;  // Suppress unused variable warning
    (void)val2;  // Suppress unused variable warning
}

// Thread function for multithreaded test
void* thread_func(void* arg) {
    int thread_id = *(int*)arg;
    uint64_t start = rdtsc();
    
    // Each thread calls functions
    call_many_functions();
    
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;
    
    printf("  Thread %d: %lu cycles\n", thread_id, cycles);
    return NULL;
}

int main(int argc, char *argv[]) {
    printf("=== Dynamic Linking Test Program ===\n");
    
    // Check binding mode
    void *handle = dlopen(NULL, RTLD_LAZY | RTLD_NOLOAD);
    if (handle) {
        Dl_info info;
        if (dladdr(main, &info)) {
            printf("Binary: %s\n", info.dli_fname);
        }
        dlclose(handle);
    }
    
    printf("\n--- First Call Timing ---\n");
    // Measure first call (includes PLT resolution for lazy binding)
    uint64_t start = rdtsc();
    call_many_functions();
    uint64_t end = rdtsc();
    
    uint64_t first_call_cycles = end - start;
    printf("First call: %lu cycles\n", first_call_cycles);
    
    printf("\n--- Subsequent Call Timing ---\n");
    // Measure subsequent call (no PLT resolution)
    start = rdtsc();
    call_many_functions();
    end = rdtsc();
    
    uint64_t second_call_cycles = end - start;
    printf("Second call: %lu cycles\n", second_call_cycles);
    
    // Calculate overhead
    if (first_call_cycles > second_call_cycles) {
        uint64_t overhead = first_call_cycles - second_call_cycles;
        double overhead_pct = (overhead * 100.0) / first_call_cycles;
        printf("\nPLT resolution overhead: %lu cycles (%.2f%%)\n", 
               overhead, overhead_pct);
    }
    
    // Multithreaded test
    if (argc > 1 && strcmp(argv[1], "--threads") == 0) {
        printf("\n--- Multithreaded Test (4 threads) ---\n");
        pthread_t threads[4];
        int thread_ids[4];
        
        for (int i = 0; i < 4; i++) {
            thread_ids[i] = i;
            if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
                perror("pthread_create");
                exit(1);
            }
        }
        
        for (int i = 0; i < 4; i++) {
            pthread_join(threads[i], NULL);
        }
    }
    
    return 0;
}
