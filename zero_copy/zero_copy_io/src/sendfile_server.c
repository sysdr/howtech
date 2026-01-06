#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT 8081

static inline unsigned long long rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

long long sendfile_transfer(int sock_fd, const char* filename) {
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
    
    off_t offset = 0;
    ssize_t sent;
    long long total_sent = 0;
    unsigned long long start_cycles, end_cycles;
    
    start_cycles = rdtsc();
    
    while (offset < file_stat.st_size) {
        sent = sendfile(sock_fd, file_fd, &offset, file_stat.st_size - offset);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            perror("sendfile");
            close(file_fd);
            return -1;
        }
        total_sent += sent;
    }
    
    end_cycles = rdtsc();
    
    close(file_fd);
    
    printf("sendfile() zero-copy:\n");
    printf("  Bytes transferred: %lld\n", total_sent);
    printf("  CPU cycles: %llu\n", end_cycles - start_cycles);
    
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
    
    printf("sendfile() server listening on port %d...\n", PORT);
    
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(listen_fd);
        return 1;
    }
    
    sendfile_transfer(client_fd, argv[1]);
    
    close(client_fd);
    close(listen_fd);
    
    return 0;
}
