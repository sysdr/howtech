#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

#define FILE_SIZE (16 * 1024 * 1024)  // 16MB

static inline long long get_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

void die(const char *msg) {
    perror(msg);
    exit(1);
}

void benchmark_msync(int sync_mode, const char *mode_name, int dirty_pages) {
    const char *filename = "msync_bench.dat";
    
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die("open");
    
    if (ftruncate(fd, FILE_SIZE) < 0) die("ftruncate");
    
    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) die("mmap");
    
    // Make pages dirty
    for (int i = 0; i < dirty_pages; i++) {
        mapped[i * 4096] = 'X';
    }
    
    // Benchmark msync
    long long start = get_usec();
    if (msync(mapped, dirty_pages * 4096, sync_mode) < 0) die("msync");
    long long latency = get_usec() - start;
    
    printf("  %-12s | %5d pages | %8lld μs | %6.2f ms\n",
           mode_name, dirty_pages, latency, latency / 1000.0);
    
    munmap(mapped, FILE_SIZE);
    close(fd);
    unlink(filename);
}

int main(void) {
    printf("\n\033[1;36m=== msync() Latency Benchmark ===\033[0m\n\n");
    printf("  Mode         | Pages      | Latency    | ms\n");
    printf("  ─────────────────────────────────────────────────────\n");
    
    int test_sizes[] = {10, 100, 1000, 4000};
    size_t test_count = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t i = 0; i < test_count; i++) {
        benchmark_msync(MS_ASYNC, "MS_ASYNC", test_sizes[i]);
    }
    
    printf("  ─────────────────────────────────────────────────────\n");
    
    for (size_t i = 0; i < test_count; i++) {
        benchmark_msync(MS_SYNC, "MS_SYNC", test_sizes[i]);
    }
    
    printf("\n\033[1;33mNote:\033[0m MS_SYNC blocks until pages hit disk.\n");
    printf("       MS_ASYNC just schedules writeback.\n\n");
    
    return 0;
}
