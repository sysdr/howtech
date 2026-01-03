#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

// External Rust function (DANGER: Not async-signal-safe!)
extern void rust_cleanup(void);

static volatile sig_atomic_t signal_count = 0;
static volatile sig_atomic_t deadlock_likely = 0;

// DANGEROUS: This handler calls Rust code that allocates
void dangerous_signal_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    signal_count++;
    
    // This is UNSAFE - calling Rust code from signal handler
    // If main thread is in malloc, this will deadlock
    rust_cleanup();
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = dangerous_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    
    printf("Dangerous example: PID %d\n", getpid());
    printf("Signal handler will call Rust code (UNSAFE!)\n");
    printf("Send signals with: kill -USR1 %d\n\n", getpid());
    printf("Watch for deadlock when signal interrupts allocation...\n\n");
    
    // Simulate workload that allocates frequently
    for (int i = 0; i < 100; i++) {
        // Allocate and free to increase chance of signal hitting malloc
        char *data = malloc(1024 * 1024); // 1MB allocation
        if (!data) {
            fprintf(stderr, "malloc failed\n");
            return 1;
        }
        
        memset(data, 0, 1024 * 1024);
        
        printf("Iteration %d: allocated 1MB, signals received: %d\n", i, signal_count);
        
        free(data);
        usleep(100000); // 100ms - window for signal to arrive
    }
    
    printf("\nCompleted %d iterations, received %d signals\n", 100, signal_count);
    printf("If you see this, we got lucky (no deadlock)\n");
    
    return 0;
}
