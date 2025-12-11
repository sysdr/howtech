#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FILE_SIZE (64 * 1024 * 1024)  // 64MB
#define ITERATIONS 100

void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(void) {
    const char *filename = "dirty_test.dat";
    
    printf("\033[1;36m=== Dirty Page Generator ===\033[0m\n");
    printf("Creating %d MB of dirty pages...\n\n", FILE_SIZE / 1024 / 1024);
    
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) die("open");
    
    if (ftruncate(fd, FILE_SIZE) < 0) die("ftruncate");
    
    char *mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) die("mmap");
    
    // Write to create dirty pages
    for (int i = 0; i < ITERATIONS; i++) {
        size_t offset = (rand() % (FILE_SIZE / 4096)) * 4096;
        mapped[offset] = 'X';
        
        if (i % 10 == 0) {
            printf("  Written %d pages...\r", i);
            fflush(stdout);
        }
        usleep(10000);  // 10ms delay
    }
    
    printf("\n\n\033[1;32m✓ Created %d dirty pages\033[0m\n", ITERATIONS);
    printf("Watch them in the monitor! They'll flush in ~30 seconds.\n");
    printf("Or press Ctrl+C and the munmap() will flush them synchronously.\n\n");
    
    // Keep mapping alive so pages stay dirty
    printf("Keeping pages dirty... (Press Ctrl+C to cleanup)\n");
    pause();
    
    munmap(mapped, FILE_SIZE);
    close(fd);
    unlink(filename);
    
    return 0;
}
