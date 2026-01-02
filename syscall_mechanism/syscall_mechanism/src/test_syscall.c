#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#define DEVICE_PATH "/dev/custom_syscall"

struct syscall_params {
    unsigned long arg1;
    unsigned long arg2;
    char buffer[256];
    size_t len;
};

/* Measure syscall overhead */
static long long measure_operation(int fd, unsigned long arg1, unsigned long arg2)
{
    struct timespec start, end;
    struct syscall_params params;
    long result;
    
    params.arg1 = arg1;
    params.arg2 = arg2;
    snprintf(params.buffer, sizeof(params.buffer), "Test input data");
    params.len = sizeof(params.buffer);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    result = ioctl(fd, 0, &params);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long long ns = (end.tv_sec - start.tv_sec) * 1000000000LL + 
                   (end.tv_nsec - start.tv_nsec);
    
    if (result < 0) {
        fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("  Result: %ld, Response: \"%s\", Time: %lld ns\n", 
           result, params.buffer, ns);
    
    return ns;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    int fd;
    int i;
    long long total_time = 0;
    int iterations = 100;
    
    printf("Custom Syscall Test\n");
    printf("===================\n\n");
    
    /* Open device */
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        fprintf(stderr, "Make sure the kernel module is loaded and device exists\n");
        return 1;
    }
    
    printf("Testing custom syscall-like operation...\n\n");
    
    /* Run some tests */
    printf("Test 1: Basic operation\n");
    measure_operation(fd, 42, 58);
    printf("\n");
    
    printf("Test 2: Different values\n");
    measure_operation(fd, 1000, 2000);
    printf("\n");
    
    printf("Test 3: Large numbers\n");
    measure_operation(fd, 999999, 1);
    printf("\n");
    
    /* Benchmark */
    printf("Benchmarking (n=%d)...\n", iterations);
    for (i = 0; i < iterations; i++) {
        long long ns = measure_operation(fd, i, i * 2);
        if (ns > 0) {
            total_time += ns;
        }
    }
    
    printf("\nBenchmark Results:\n");
    printf("  Average time: %lld ns per call\n", total_time / iterations);
    printf("  Total time: %.2f ms\n", total_time / 1000000.0);
    printf("  Throughput: %.0f ops/sec\n", 
           iterations / (total_time / 1000000000.0));
    
    close(fd);
    
    return 0;
}
