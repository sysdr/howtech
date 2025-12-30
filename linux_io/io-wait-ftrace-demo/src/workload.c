#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#define BLOCK_SIZE 4096
#define NUM_BLOCKS 1000

static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static void create_io_workload(const char *filename, int use_direct, int do_sync) {
    int flags = O_WRONLY | O_CREAT | O_TRUNC;
    if (use_direct) {
        flags |= O_DIRECT | O_SYNC;
    }
    
    int fd = open(filename, flags, 0644);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return;
    }
    
    // Allocate aligned buffer for O_DIRECT
    void *buffer = NULL;
    if (posix_memalign(&buffer, BLOCK_SIZE, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Failed to allocate aligned buffer\n");
        close(fd);
        return;
    }
    memset(buffer, 0xAB, BLOCK_SIZE);
    
    printf("\n=== Starting I/O Workload ===\n");
    printf("Mode: %s\n", use_direct ? "O_DIRECT (bypass page cache)" : "Buffered (page cache)");
    printf("Blocks: %d x %d bytes\n", NUM_BLOCKS, BLOCK_SIZE);
    
    double start = get_time_ms();
    
    for (int i = 0; i < NUM_BLOCKS; i++) {
        ssize_t written = write(fd, buffer, BLOCK_SIZE);
        if (written != BLOCK_SIZE) {
            fprintf(stderr, "Write failed at block %d: %s\n", i, strerror(errno));
            break;
        }
        
        // Add some delay to make tracing easier to observe
        if (i % 100 == 0) {
            usleep(10000); // 10ms pause every 100 blocks
        }
    }
    
    if (do_sync) {
        printf("Calling fsync() to force write to disk...\n");
        if (fsync(fd) < 0) {
            fprintf(stderr, "fsync failed: %s\n", strerror(errno));
        }
    }
    
    double end = get_time_ms();
    
    free(buffer);
    close(fd);
    
    printf("Completed in %.2f ms\n", end - start);
    printf("Throughput: %.2f MB/s\n", 
           (NUM_BLOCKS * BLOCK_SIZE / 1024.0 / 1024.0) / ((end - start) / 1000.0));
}

int main(int argc, char *argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;   // Suppress unused parameter warning
    printf("I/O Workload Generator for Ftrace Demo\n");
    printf("=======================================\n");
    
    const char *testfile = "/tmp/iotest.dat";
    
    // Test 1: Buffered writes (fast, uses page cache)
    printf("\n--- Test 1: Buffered Writes ---\n");
    create_io_workload(testfile, 0, 0);
    unlink(testfile);
    
    sleep(2);
    
    // Test 2: Direct I/O (bypasses page cache, shows real disk latency)
    printf("\n--- Test 2: Direct I/O (O_DIRECT) ---\n");
    create_io_workload(testfile, 1, 0);
    unlink(testfile);
    
    sleep(2);
    
    // Test 3: Buffered with fsync (flushes page cache to disk)
    printf("\n--- Test 3: Buffered + fsync() ---\n");
    create_io_workload(testfile, 0, 1);
    unlink(testfile);
    
    return 0;
}
