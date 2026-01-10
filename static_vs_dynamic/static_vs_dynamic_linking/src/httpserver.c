#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 4096

volatile sig_atomic_t keep_running = 1;

void signal_handler(int signum) {
    (void)signum;  // Suppress unused parameter warning
    keep_running = 0;
}

long get_monotonic_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    long start_time = get_monotonic_ns();
    printf("[STARTUP] Static binary initialization: %ld ns\n", get_monotonic_ns() - start_time);
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt SO_REUSEPORT");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("[SERVER] Listening on port %d (PID: %d)\n", PORT, getpid());
    printf("[SERVER] Memory mapping: /proc/%d/maps\n", getpid());
    
    // Accept connections
    int request_count = 0;
    while (keep_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }
        
        if (activity == 0) continue;
        
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            if (errno != EINTR) perror("accept");
            continue;
        }
        
        request_count++;
        long req_start = get_monotonic_ns();
        
        ssize_t valread = read(new_socket, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("read");
            close(new_socket);
            continue;
        }
        
        const char *response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Static Binary Response\n"
            "Built with musl libc\n"
            "Zero runtime dependencies\n";
        
        ssize_t written = write(new_socket, response, strlen(response));
        if (written < 0) {
            perror("write");
        }
        close(new_socket);
        
        long req_time = get_monotonic_ns() - req_start;
        printf("[REQUEST %d] Handled in %ld ns (%.2f μs)\n", 
               request_count, req_time, req_time / 1000.0);
    }
    
    printf("\n[SHUTDOWN] Graceful shutdown after %d requests\n", request_count);
    close(server_fd);
    return 0;
}
