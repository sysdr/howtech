/*
 * Demonstration program with intentional syscall failures
 * Shows various error conditions for strace filtering
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_RESET   "\x1b[0m"

void test_file_operations(void) {
    printf(ANSI_YELLOW "\n[Test 1] File Operations - Some Will Fail\n" ANSI_RESET);
    
    // This will fail - file doesn't exist
    int fd = open("/tmp/nonexistent_config.json", O_RDONLY);
    if (fd < 0) {
        printf(ANSI_RED "  ✗ Failed to open /tmp/nonexistent_config.json: %s\n" ANSI_RESET, 
               strerror(errno));
    }
    
    // This will succeed
    fd = open("/etc/hosts", O_RDONLY);
    if (fd >= 0) {
        printf(ANSI_GREEN "  ✓ Successfully opened /etc/hosts (fd=%d)\n" ANSI_RESET, fd);
        close(fd);
    }
    
    // This will fail - permission denied (usually)
    fd = open("/etc/shadow", O_RDONLY);
    if (fd < 0) {
        printf(ANSI_RED "  ✗ Failed to open /etc/shadow: %s\n" ANSI_RESET, 
               strerror(errno));
    }
    
    // This will fail - not a directory
    fd = open("/etc/hosts/invalid", O_RDONLY);
    if (fd < 0) {
        printf(ANSI_RED "  ✗ Failed to open /etc/hosts/invalid: %s\n" ANSI_RESET, 
               strerror(errno));
    }
    
    // Stat call that will fail
    struct stat st;
    if (stat("/var/app/missing.conf", &st) < 0) {
        printf(ANSI_RED "  ✗ Failed to stat /var/app/missing.conf: %s\n" ANSI_RESET, 
               strerror(errno));
    }
}

void test_network_operations(void) {
    printf(ANSI_YELLOW "\n[Test 2] Network Operations - Connection Failures\n" ANSI_RESET);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(ANSI_RED "  ✗ Failed to create socket: %s\n" ANSI_RESET, strerror(errno));
        return;
    }
    
    printf(ANSI_GREEN "  ✓ Created socket (fd=%d)\n" ANSI_RESET, sock);
    
    // Try to connect to localhost on closed port - will fail with ECONNREFUSED
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);  // Likely not listening
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf(ANSI_RED "  ✗ Failed to connect to 127.0.0.1:9999: %s\n" ANSI_RESET, 
               strerror(errno));
    }
    
    close(sock);
    
    // Create socket and try to bind to privileged port - will fail
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        addr.sin_port = htons(80);  // Privileged port
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf(ANSI_RED "  ✗ Failed to bind to port 80: %s\n" ANSI_RESET, 
                   strerror(errno));
        }
        close(sock);
    }
}

void test_successful_operations(void) {
    printf(ANSI_YELLOW "\n[Test 3] Successful Operations (Noise)\n" ANSI_RESET);
    
    // These all succeed - they're noise when debugging failures
    int fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) {
        ssize_t written = write(fd, "test", 4);
        if (written >= 0) {
            printf(ANSI_GREEN "  ✓ Successfully wrote to /dev/null\n" ANSI_RESET);
        }
        close(fd);
    }
    
    // Multiple successful reads
    fd = open("/etc/hostname", O_RDONLY);
    if (fd >= 0) {
        char buf[256];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf(ANSI_GREEN "  ✓ Read hostname: %s" ANSI_RESET, buf);
        }
        close(fd);
    }
}

void test_write_failures(void) {
    printf(ANSI_YELLOW "\n[Test 4] Write Failures\n" ANSI_RESET);
    
    // Try to write to read-only file descriptor
    int fd = open("/etc/hosts", O_RDONLY);
    if (fd >= 0) {
        if (write(fd, "test", 4) < 0) {
            printf(ANSI_RED "  ✗ Failed to write to read-only fd: %s\n" ANSI_RESET, 
                   strerror(errno));
        }
        close(fd);
    }
    
    // Write to closed fd
    if (write(999, "test", 4) < 0) {
        printf(ANSI_RED "  ✗ Failed to write to invalid fd 999: %s\n" ANSI_RESET, 
               strerror(errno));
    }
}

int main(void) {
    printf(ANSI_GREEN "╔═══════════════════════════════════════════════════╗\n");
    printf("║  Syscall Failure Demonstration Program          ║\n");
    printf("║  Run with: strace -e status=failed -y ./program ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n" ANSI_RESET);
    
    test_file_operations();
    test_network_operations();
    test_successful_operations();
    test_write_failures();
    
    printf(ANSI_GREEN "\n✓ All tests completed\n" ANSI_RESET);
    printf(ANSI_YELLOW "Now try running with different strace filters!\n" ANSI_RESET);
    
    return 0;
}
