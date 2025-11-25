#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define REFRESH_MS 1000

volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

typedef struct {
    long voluntary_switches;
    long involuntary_switches;
} context_stats_t;

int read_context_switches(pid_t pid, context_stats_t *stats) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
            sscanf(line + 24, "%ld", &stats->voluntary_switches);
        } else if (strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
            sscanf(line + 27, "%ld", &stats->involuntary_switches);
        }
    }
    
    fclose(fp);
    return 0;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid = atoi(argv[1]);
    signal(SIGINT, signal_handler);
    
    context_stats_t prev = {0}, curr = {0};
    read_context_switches(target_pid, &prev);
    
    printf("\033[?25l"); // Hide cursor
    
    while (keep_running) {
        usleep(REFRESH_MS * 1000);
        
        if (read_context_switches(target_pid, &curr) != 0) {
            break;
        }
        
        clear_screen();
        
        printf("╔═══════════════════════════════════════════════════════════╗\n");
        printf("║       SYSCALL ACTIVITY MONITOR - PID %d                 ║\n", target_pid);
        printf("╚═══════════════════════════════════════════════════════════╝\n\n");
        
        long vol_delta = curr.voluntary_switches - prev.voluntary_switches;
        long invol_delta = curr.involuntary_switches - prev.involuntary_switches;
        
        printf("Context Switches (per second):\n");
        printf("  Voluntary:     %6ld  (syscalls that block)\n", vol_delta);
        printf("  Involuntary:   %6ld  (preempted by scheduler)\n", invol_delta);
        printf("  Total:         %6ld\n\n", vol_delta + invol_delta);
        
        printf("Total Since Start:\n");
        printf("  Voluntary:     %ld\n", curr.voluntary_switches);
        printf("  Involuntary:   %ld\n\n", curr.involuntary_switches);
        
        printf("Press Ctrl+C to exit\n");
        
        prev = curr;
    }
    
    printf("\033[?25h"); // Show cursor
    printf("\n");
    
    return 0;
}
