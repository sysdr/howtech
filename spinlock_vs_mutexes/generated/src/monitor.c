#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_CYAN "\033[36m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED "\033[31m"
#define COLOR_RESET "\033[0m"

typedef struct {
    unsigned long utime;
    unsigned long stime;
    unsigned long voluntary_switches;
    unsigned long involuntary_switches;
} proc_stats_t;

int read_proc_stat(pid_t pid, proc_stats_t *stats) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    
    if (fscanf(f, "%*d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu",
               comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
               &flags, &minflt, &cminflt, &majflt, &cmajflt,
               &stats->utime, &stats->stime) < 14) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    // Read context switches
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
            sscanf(line + 24, "%lu", &stats->voluntary_switches);
        } else if (strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
            sscanf(line + 27, "%lu", &stats->involuntary_switches);
        }
    }
    
    fclose(f);
    return 0;
}

void print_header() {
    printf(COLOR_CYAN);
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║           SPINLOCK vs MUTEX PERFORMANCE MONITOR                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <spinlock_pid> <mutex_pid>\n", argv[0]);
        return 1;
    }
    
    pid_t spin_pid = atoi(argv[1]);
    pid_t mutex_pid = atoi(argv[2]);
    
    proc_stats_t spin_stats_prev = {0}, spin_stats_curr = {0};
    proc_stats_t mutex_stats_prev = {0}, mutex_stats_curr = {0};
    
    read_proc_stat(spin_pid, &spin_stats_prev);
    read_proc_stat(mutex_pid, &mutex_stats_prev);
    
    while (1) {
        sleep(1);
        
        if (read_proc_stat(spin_pid, &spin_stats_curr) != 0 ||
            read_proc_stat(mutex_pid, &mutex_stats_curr) != 0) {
            break;  // Process terminated
        }
        
        unsigned long spin_utime_delta = spin_stats_curr.utime - spin_stats_prev.utime;
        unsigned long spin_stime_delta = spin_stats_curr.stime - spin_stats_prev.stime;
        unsigned long spin_vol_delta = spin_stats_curr.voluntary_switches - 
                                       spin_stats_prev.voluntary_switches;
        unsigned long spin_invol_delta = spin_stats_curr.involuntary_switches - 
                                         spin_stats_prev.involuntary_switches;
        
        unsigned long mutex_utime_delta = mutex_stats_curr.utime - mutex_stats_prev.utime;
        unsigned long mutex_stime_delta = mutex_stats_curr.stime - mutex_stats_prev.stime;
        unsigned long mutex_vol_delta = mutex_stats_curr.voluntary_switches - 
                                        mutex_stats_prev.voluntary_switches;
        unsigned long mutex_invol_delta = mutex_stats_curr.involuntary_switches - 
                                          mutex_stats_prev.involuntary_switches;
        
        printf(CLEAR_SCREEN);
        print_header();
        printf("\n");
        
        printf(COLOR_YELLOW "SPINLOCK (PID %d)" COLOR_RESET "\n", spin_pid);
        printf("  CPU Time:          User: %3lu ticks  System: %3lu ticks\n", 
               spin_utime_delta, spin_stime_delta);
        printf("  Context Switches:  Vol: %4lu  Invol: %4lu\n",
               spin_vol_delta, spin_invol_delta);
        printf("  " COLOR_RED "Status:            BUSY-WAITING (100%% CPU)" COLOR_RESET "\n");
        printf("\n");
        
        printf(COLOR_GREEN "MUTEX (PID %d)" COLOR_RESET "\n", mutex_pid);
        printf("  CPU Time:          User: %3lu ticks  System: %3lu ticks\n",
               mutex_utime_delta, mutex_stime_delta);
        printf("  Context Switches:  Vol: %4lu  Invol: %4lu\n",
               mutex_vol_delta, mutex_invol_delta);
        printf("  " COLOR_GREEN "Status:            SLEEPING when blocked" COLOR_RESET "\n");
        printf("\n");
        
        printf(COLOR_CYAN "OBSERVATIONS:" COLOR_RESET "\n");
        if (spin_utime_delta + spin_stime_delta > mutex_utime_delta + mutex_stime_delta) {
            printf("  ⚠  Spinlock using more CPU (busy-waiting)\n");
        }
        if (mutex_vol_delta > spin_vol_delta * 2) {
            printf("  ℹ  Mutex has more context switches (expected)\n");
        }
        
        spin_stats_prev = spin_stats_curr;
        mutex_stats_prev = mutex_stats_curr;
    }
    
    return 0;
}
