#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT 8082
#define PIPE_SIZE (1024 * 64)  // 64KB pipe buffer

static inline unsigned long long rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

long long splice_transfer(int sock_fd, const char* filename) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        return -1;
    }
    
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) < 0) {
        perror("fstat");
        close(file_fd);
        return -1;
    }
    
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        close(file_fd);
        return -1;
    }
    
    // Try to set larger pipe buffer
    fcntl(pipe_fd[1], F_SETPIPE_SZ, PIPE_SIZE);
    
    long long total_sent = 0;
    unsigned long long start_cycles, end_cycles;
    
    start_cycles = rdtsc();
    end_cycles = start_cycles;  // Initialize to avoid uninitialized variable warning
    
    off_t remaining = file_stat.st_size;
    
    while (remaining > 0) {
        // File → Pipe (adds page references)
        ssize_t spliced = splice(file_fd, NULL, pipe_fd[1], NULL, 
                                 remaining, SPLICE_F_MOVE);
        if (spliced <= 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            perror("splice (file->pipe)");
            break;
        }
        
        // Pipe → Socket (transfers page references)
        ssize_t sent = 0;
        while (sent < spliced) {
            ssize_t s = splice(pipe_fd[0], NULL, sock_fd, NULL,
                              spliced - sent, SPLICE_F_MOVE);
            if (s <= 0) {
                if (errno == EAGAIN || errno == EINTR) {
                    continue;
                }
                perror("splice (pipe->socket)");
                goto cleanup;
            }
            sent += s;
        }
        
        total_sent += sent;
        remaining -= sent;
    }
    
    end_cycles = rdtsc();
    
    printf("splice() zero-copy:\n");
    printf("  Bytes transferred: %lld\n", total_sent);
    printf("  CPU cycles: %llu\n", end_cycles - start_cycles);
    
cleanup:
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    close(file_fd);
    
    return end_cycles - start_cycles;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    
    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    
    printf("splice() server listening on port %d...\n", PORT);
    
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(listen_fd);
        return 1;
    }
    
    splice_transfer(client_fd, argv[1]);
    
    close(client_fd);
    close(listen_fd);
    
    return 0;
}
