#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    int fd = open(path, O_RDONLY);
    
    if (fd < 0) {
        if (errno == EPERM) {
            printf("❌ ACCESS DENIED by LSM BPF policy (EPERM)\n");
            printf("   File: %s\n", path);
            printf("   UID: %u\n", getuid());
            return 1;
        }
        perror("open");
        return 1;
    }

    printf("✓ ACCESS GRANTED\n");
    printf("  File descriptor: %d\n", fd);
    
    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("  First %zd bytes: %s\n", n, buf);
    }
    
    close(fd);
    return 0;
}
