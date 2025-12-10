#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

static volatile int running = 1;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

typedef struct {
    unsigned long voluntary_switches;
    unsigned long nonvoluntary_switches;
    unsigned long utime;
    unsigned long stime;
} proc_stats_t;

int read_proc_stats(pid_t pid, proc_stats_t *stats) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "voluntary_ctxt_switches: %lu", &stats->voluntary_switches) == 1) continue;
        if (sscanf(line, "nonvoluntary_ctxt_switches: %lu", &stats->nonvoluntary_switches) == 1) continue;
    }
    fclose(f);
    
    // Read CPU times from /proc/[pid]/stat
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) return -1;
    
    if (fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
           &stats->utime, &stats->stime) != 2) {
        fclose(f);
        return -1;
    }
    fclose(f);
    
    return 0;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void print_header(void) {
    printf(BOLD CYAN "╔══════════════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD CYAN "║" RESET "           " BOLD "IPC Performance Benchmark Monitor" RESET "              " BOLD CYAN "║\n" RESET);
    printf(BOLD CYAN "╚══════════════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void print_comparison(const char *name, proc_stats_t *prev, proc_stats_t *curr) {
    (void)prev;  // Suppress unused parameter warning for prev
    unsigned long user_delta = curr->utime - prev->utime;
    unsigned long sys_delta = curr->stime - prev->stime;
    
    printf(BOLD "%s:\n" RESET, name);
    printf("  Context Switches:  " YELLOW "%8lu" RESET " voluntary  " RED "%8lu" RESET " non-voluntary\n",
           curr->voluntary_switches, curr->nonvoluntary_switches);
    printf("  CPU Time (ticks):  " GREEN "User: %8lu" RESET "  " MAGENTA "Sys: %8lu" RESET "\n",
           curr->utime, curr->stime);
    
    if (user_delta + sys_delta > 0) {
        double sys_percent = (100.0 * sys_delta) / (user_delta + sys_delta);
        printf("  System CPU%%:       " BOLD "%6.1f%%" RESET "\n", sys_percent);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <pid1> <pid2>\n", argv[0]);
        return 1;
    }
    
    pid_t pid1 = atoi(argv[1]);
    pid_t pid2 = atoi(argv[2]);
    
    signal(SIGINT, sigint_handler);
    
    proc_stats_t shm_prev = {0}, shm_curr = {0};
    proc_stats_t mq_prev = {0}, mq_curr = {0};
    
    // Initial read
    read_proc_stats(pid1, &shm_prev);
    read_proc_stats(pid2, &mq_prev);
    
    while (running) {
        sleep(1);
        
        if (read_proc_stats(pid1, &shm_curr) != 0 ||
            read_proc_stats(pid2, &mq_curr) != 0) {
            break; // Processes finished
        }
        
        clear_screen();
        print_header();
        
        printf(BOLD BLUE "═══ Shared Memory (Lock-Free Ring Buffer) ═══\n" RESET);
        print_comparison("Shared Memory", &shm_prev, &shm_curr);
        
        printf(BOLD BLUE "═══ POSIX Message Queue ═══\n" RESET);
        print_comparison("Message Queue", &mq_prev, &mq_curr);
        
        printf(BOLD GREEN "Press Ctrl+C to exit monitor\n" RESET);
        
        shm_prev = shm_curr;
        mq_prev = mq_curr;
    }
    
    printf("\n" BOLD "Monitoring complete.\n" RESET);
    return 0;
}
