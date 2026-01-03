#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_TITLE   "\033[1;36m"
#define COLOR_GOOD    "\033[0;32m"
#define COLOR_WARN    "\033[1;33m"
#define COLOR_BAD     "\033[0;31m"
#define COLOR_INFO    "\033[0;34m"

static volatile sig_atomic_t signals_received = 0;
static volatile sig_atomic_t last_signal = 0;

void safe_handler(int sig) {
    signals_received++;
    last_signal = sig;
}

void print_header(void) {
    printf("\033[2J\033[H");  // Clear screen
    printf(COLOR_TITLE "╔══════════════════════════════════════════════════════════════╗\n");
    printf("║          SIGNAL HANDLING SAFETY MONITOR                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
}

void print_signal_info(pid_t target_pid) {
    char path[256];
    char line[512];
    
    printf("\n" COLOR_INFO "Process Information:" COLOR_RESET "\n");
    printf("  PID: %d\n", target_pid);
    
    // Read signal info from /proc
    snprintf(path, sizeof(path), "/proc/%d/status", target_pid);
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "SigQ:", 5) == 0 ||
                strncmp(line, "SigPnd:", 7) == 0 ||
                strncmp(line, "SigBlk:", 7) == 0 ||
                strncmp(line, "SigIgn:", 7) == 0 ||
                strncmp(line, "SigCgt:", 7) == 0) {
                printf("  %s", line);
            }
        }
        fclose(f);
    }
}

void print_statistics(void) {
    printf("\n" COLOR_GOOD "Signal Statistics:" COLOR_RESET "\n");
    printf("  Signals Received: %d\n", signals_received);
    if (last_signal > 0) {
        printf("  Last Signal: %d\n", last_signal);
    }
}

void print_safety_tips(void) {
    printf("\n" COLOR_WARN "Signal Safety Rules:" COLOR_RESET "\n");
    printf("  ✓ SAFE:   write(), _exit(), sig_atomic_t, atomics\n");
    printf("  ✗ UNSAFE: malloc, printf, mutex, Rust allocations\n");
    printf("  ✓ SAFE:   signalfd + event loop (eliminates all issues)\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid_to_monitor>\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid = atoi(argv[1]);
    
    // Set up our own safe signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = safe_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    
    printf("Monitoring process %d\n", target_pid);
    printf("Press Ctrl+C to exit\n");
    sleep(2);
    
    while (1) {
        print_header();
        print_signal_info(target_pid);
        print_statistics();
        print_safety_tips();
        
        printf("\n" COLOR_INFO "Monitoring active..." COLOR_RESET " (updates every 2s)\n");
        printf("Send test signal: kill -USR1 %d\n", getpid());
        
        sleep(2);
    }
    
    return 0;
}
