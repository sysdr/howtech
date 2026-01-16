#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

// Simulate a leaking application that doesn't close files
void leak_regular_files(int count) {
    char filename[256];
    for (int i = 0; i < count; i++) {
        snprintf(filename, sizeof(filename), "/tmp/leaked_file_%d.txt", i);
        int fd = open(filename, O_CREAT | O_RDWR, 0644);
        if (fd < 0) {
            if (errno == EMFILE) {
                printf("[EMFILE] Hit file descriptor limit at %d files\n", i);
                return;
            }
            perror("open");
            return;
        }
        // Write some data
        const char *data = "This file descriptor was never closed!\n";
        ssize_t written = write(fd, data, strlen(data));
        (void)written; // Suppress unused variable warning
        // INTENTIONALLY NOT CLOSING: close(fd);
        
        if (i % 10 == 0) {
            printf("Leaked %d file descriptors so far...\n", i + 1);
        }
    }
}

// Create deleted file scenario
void create_deleted_file_scenario() {
    const char *filename = "/tmp/deleted_but_open.txt";
    int fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    // Write 10MB of data
    char buffer[1024];
    memset(buffer, 'A', sizeof(buffer));
    for (int i = 0; i < 10240; i++) {
        ssize_t written = write(fd, buffer, sizeof(buffer));
        (void)written; // Suppress unused variable warning
    }
    
    printf("Created file: %s (10MB)\n", filename);
    printf("File descriptor: %d\n", fd);
    
    // Delete the file while keeping it open
    if (unlink(filename) == 0) {
        printf("File DELETED but fd %d still open - space NOT freed!\n", fd);
    }
    
    // File remains accessible through fd even though deleted
    off_t size = lseek(fd, 0, SEEK_END);
    printf("File still accessible via fd: size = %ld bytes\n", size);
    
    // Keep fd open - this prevents disk space reclamation
    printf("Sleeping 60 seconds (fd stays open)...\n");
    sleep(60);
    
    close(fd);
    printf("Finally closed fd - NOW disk space can be freed\n");
}

// Count current FDs for this process
int count_open_fds() {
    DIR *dir = opendir("/proc/self/fd");
    if (!dir) {
        perror("opendir");
        return -1;
    }
    
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            count++;
        }
    }
    closedir(dir);
    return count;
}

// Show current fd info
void show_fd_info() {
    printf("\n=== Current FD Status ===\n");
    printf("PID: %d\n", getpid());
    printf("Open FDs: %d\n", count_open_fds());
    printf("\nTo inspect with lsof:\n");
    printf("  lsof -p %d\n", getpid());
    printf("\nTo inspect /proc directly:\n");
    printf("  ls -la /proc/%d/fd/\n", getpid());
    printf("  cat /proc/%d/fdinfo/3\n", getpid());
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        printf("Modes:\n");
        printf("  leak <count>  - Leak file descriptors\n");
        printf("  deleted       - Create deleted-but-open file scenario\n");
        printf("  monitor       - Monitor FD count\n");
        return 1;
    }
    
    if (strcmp(argv[1], "leak") == 0) {
        int count = argc > 2 ? atoi(argv[2]) : 50;
        printf("Leaking %d file descriptors...\n", count);
        show_fd_info();
        leak_regular_files(count);
        show_fd_info();
        printf("Process staying alive for 30 seconds...\n");
        sleep(30);
    } else if (strcmp(argv[1], "deleted") == 0) {
        printf("=== Deleted File Scenario ===\n");
        show_fd_info();
        create_deleted_file_scenario();
        show_fd_info();
    } else if (strcmp(argv[1], "monitor") == 0) {
        printf("Monitoring FD count for 60 seconds...\n");
        for (int i = 0; i < 60; i++) {
            printf("[%2d] FDs: %d\n", i, count_open_fds());
            sleep(1);
        }
    }
    
    return 0;
}
