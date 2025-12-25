#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

// Function we'll probe with uprobe
__attribute__((noinline)) void my_function(int value) {
    printf("  my_function called with value: %d\n", value);
    usleep(10000); // 10ms
}

// Function to do some file I/O (triggers tracepoints)
void do_file_operations(int iteration) {
    char filename[256];
    snprintf(filename, sizeof(filename), "/tmp/test_file_%d.txt", iteration);
    
    // open() triggers sys_enter_openat and sys_exit_openat tracepoints
    int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    // write() triggers syscall tracepoints
    const char *data = "Test data for tracepoint demonstration\n";
    ssize_t written = write(fd, data, strlen(data));
    (void)written; // Suppress unused variable warning
    
    // close() triggers syscall tracepoints
    close(fd);
    
    // unlink() triggers vfs tracepoints
    unlink(filename);
}

// Function that allocates memory (we'll trace with uprobe on malloc)
void do_allocations(int count) {
    for (int i = 0; i < count; i++) {
        size_t size = (rand() % 10 + 1) * 1024; // 1-10 KB
        void *ptr = malloc(size);
        if (ptr) {
            memset(ptr, 0, size);
            free(ptr);
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    printf("Target program starting (PID: %d)\n", getpid());
    printf("This program will run for 30 seconds...\n\n");
    
    srand(time(NULL));
    
    for (int i = 0; i < 30; i++) {
        printf("Iteration %d:\n", i + 1);
        
        // Call our custom function (for uprobe demo)
        my_function(i);
        
        // Do file operations (for tracepoint demo)
        do_file_operations(i);
        
        // Do memory allocations (for malloc uprobe demo)
        do_allocations(5);
        
        sleep(1);
    }
    
    printf("\nTarget program exiting\n");
    return 0;
}
