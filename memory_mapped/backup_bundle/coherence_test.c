#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#define FILE_SIZE (4 * 1024 * 1024)  // 4MB
#define TEST_STRING "MMAP_WRITE_DATA_"
#define ITERATIONS 10

static inline long long get_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int with_sync = (argc > 1 && strcmp(argv[1], "sync") == 0);
    const char *filename = "test_coherence.dat";
    
    printf("\n\033[1;36m=== Coherence Test: mmap() write vs read() visibility ===\033[0m\n");
    printf("Mode: %s\n\n", with_sync ? "WITH msync()" : "WITHOUT msync()");
    
    // Create and truncate file
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die("open");
    
    if (ftruncate(fd, FILE_SIZE) < 0) die("ftruncate");
    
    // Parent mmaps the file
    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) die("mmap");
    
    pid_t pid = fork();
    if (pid < 0) die("fork");
    
    if (pid == 0) {
        // Child process: reads via read() syscall
        sleep(1);  // Let parent write first
        
        char read_buf[64];
        long long total_latency = 0;
        int visible_count = 0;
        
        for (int i = 0; i < ITERATIONS; i++) {
            long long start = get_usec();
            
            lseek(fd, i * 64, SEEK_SET);
            ssize_t n = read(fd, read_buf, sizeof(read_buf));
            if (n < 0) die("read");
            
            read_buf[n] = '\0';
            long long latency = get_usec() - start;
            total_latency += latency;
            
            int visible = (strncmp(read_buf, TEST_STRING, strlen(TEST_STRING)) == 0);
            if (visible) visible_count++;
            
            printf("  [Child read %2d] latency=%4lld μs | visible=%s | data='%s'\n",
                   i, latency, visible ? "\033[0;32mYES\033[0m" : "\033[0;31mNO \033[0m",
                   visible ? "MATCH" : read_buf);
            
            usleep(50000);  // 50ms between reads
        }
        
        printf("\n\033[1;33mChild Summary:\033[0m\n");
        printf("  Visible writes: %d/%d (%.1f%%)\n", 
               visible_count, ITERATIONS, 100.0 * visible_count / ITERATIONS);
        printf("  Avg read latency: %lld μs\n", total_latency / ITERATIONS);
        
        munmap(mapped, FILE_SIZE);
        close(fd);
        exit(0);
    }
    
    // Parent process: writes via mmap
    for (int i = 0; i < ITERATIONS; i++) {
        char write_str[64];
        snprintf(write_str, sizeof(write_str), "%s%04d", TEST_STRING, i);
        
        long long start = get_usec();
        memcpy(mapped + i * 64, write_str, strlen(write_str) + 1);
        
        if (with_sync) {
            // msync requires page-aligned ranges; syncing entire mapping keeps demo simple
            if (msync(mapped, FILE_SIZE, MS_SYNC) < 0) die("msync");
        }
        
        long long latency = get_usec() - start;
        
        printf("[Parent write %2d] latency=%4lld μs | %s\n",
               i, latency, with_sync ? "MS_SYNC" : "no sync");
        
        usleep(50000);  // 50ms between writes
    }
    
    wait(NULL);
    
    // Cleanup
    munmap(mapped, FILE_SIZE);
    close(fd);
    unlink(filename);
    
    printf("\n\033[1;32m✓ Test complete\033[0m\n\n");
    return 0;
}
