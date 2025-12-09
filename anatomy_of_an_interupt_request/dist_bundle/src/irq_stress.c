#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

static volatile int running = 1;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

void *network_stress(void *arg) {
    (void)arg;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);
    
    char buf[1024];
    memset(buf, 'A', sizeof(buf));
    
    printf("Network stress thread started (generating interrupts via loopback)\n");
    
    while (running) {
        sendto(sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr, sizeof(addr));
        usleep(100); // 10,000 packets/sec
    }
    
    close(sock);
    return NULL;
}

void *disk_stress(void *arg) {
    (void)arg;
    printf("Disk stress thread started (generating block I/O interrupts)\n");
    
    while (running) {
        int fd = open("/tmp/irq_test_file", O_RDWR | O_CREAT | O_SYNC, 0644);
        if (fd >= 0) {
            char buf[4096];
            memset(buf, 'B', sizeof(buf));
            ssize_t written = write(fd, buf, sizeof(buf));
            (void)written; // Ignore write errors in stress test
            fsync(fd);
            close(fd);
        }
        usleep(1000); // 1000 ops/sec
    }
    
    unlink("/tmp/irq_test_file");
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    signal(SIGINT, sigint_handler);
    
    printf("IRQ Stress Test - Generating Hardware Interrupts\n");
    printf("This will generate network and disk interrupts\n");
    printf("Monitor with: sudo ./irq_monitor\n");
    printf("Press Ctrl+C to stop\n\n");
    
    pthread_t net_thread, disk_thread;
    
    pthread_create(&net_thread, NULL, network_stress, NULL);
    pthread_create(&disk_thread, NULL, disk_stress, NULL);
    
    while (running) {
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    pthread_join(net_thread, NULL);
    pthread_join(disk_thread, NULL);
    
    return 0;
}
