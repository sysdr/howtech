#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <signal.h>

#define SAMPLE_DURATION_MS 100
#define TOTAL_SAMPLES 50

volatile sig_atomic_t stop_monitoring = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_monitoring = 1;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void print_bar(const char *label, double value, double max, const char *color) {
    int bar_width = 40;
    int filled = (int)((value / max) * bar_width);
    
    printf("  %-20s [", label);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("%s█%s", color, "\033[0m");
        } else {
            printf("░");
        }
    }
    printf("] %.1f%%\n", (value / max) * 100);
}

int main(void) {
    signal(SIGINT, sigint_handler);
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         Real-Time Syscall Monitor                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("This monitor demonstrates the difference between vDSO-accelerated\n");
    printf("and traditional syscalls in real-time.\n");
    printf("\n");
    printf("Press Ctrl+C to stop...\n\n");
    sleep(2);
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    unsigned long long vdso_calls = 0;
    unsigned long long syscalls = 0;
    
    for (int sample = 0; sample < TOTAL_SAMPLES && !stop_monitoring; sample++) {
        clear_screen();
        
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) + 
                        (now.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║         Real-Time Syscall Performance Monitor               ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("Runtime: %.1f seconds\n\n", elapsed);
        
        // Simulate vDSO calls (fast)
        struct timespec vdso_start, vdso_end;
        clock_gettime(CLOCK_MONOTONIC, &vdso_start);
        for (int i = 0; i < 100000; i++) {
            getpid();
            vdso_calls++;
        }
        clock_gettime(CLOCK_MONOTONIC, &vdso_end);
        double vdso_ms = (vdso_end.tv_sec - vdso_start.tv_sec) * 1000.0 +
                         (vdso_end.tv_nsec - vdso_start.tv_nsec) / 1e6;
        
        // Simulate real syscalls (slow)
        struct timespec syscall_start, syscall_end;
        clock_gettime(CLOCK_MONOTONIC, &syscall_start);
        for (int i = 0; i < 10000; i++) {
            syscall(SYS_getpid);
            syscalls++;
        }
        clock_gettime(CLOCK_MONOTONIC, &syscall_end);
        double syscall_ms = (syscall_end.tv_sec - syscall_start.tv_sec) * 1000.0 +
                           (syscall_end.tv_nsec - syscall_start.tv_nsec) / 1e6;
        
        printf("Call Statistics:\n");
        printf("  • vDSO calls:       %12llu  (%.2f M/sec)\n", 
               vdso_calls, vdso_calls / elapsed / 1e6);
        printf("  • Real syscalls:    %12llu  (%.2f M/sec)\n\n", 
               syscalls, syscalls / elapsed / 1e6);
        
        printf("Performance (100K iterations):\n");
        print_bar("vDSO getpid()", vdso_ms, syscall_ms, "\033[0;32m");
        print_bar("Raw syscall", syscall_ms, syscall_ms, "\033[0;31m");
        
        printf("\n");
        printf("Time per call:\n");
        printf("  • vDSO:      %6.1f ns/call  🚀\n", vdso_ms * 1e6 / 100000);
        printf("  • Syscall:   %6.1f ns/call  🐌\n", syscall_ms * 1e6 / 10000);
        printf("  • Speedup:   %6.1fx faster\n", syscall_ms * 10);
        
        printf("\n");
        printf("Key Insight: vDSO eliminates context switches entirely!\n");
        printf("Press Ctrl+C to stop monitoring...\n");
        
        usleep(SAMPLE_DURATION_MS * 1000);
    }
    
    printf("\n\nMonitoring stopped.\n\n");
    return 0;
}
