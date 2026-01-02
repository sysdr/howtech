#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_BOLD "\033[1m"

static void read_proc_stat(unsigned long *user, unsigned long *sys)
{
    FILE *f = fopen("/proc/self/stat", "r");
    if (!f) {
        *user = 0;
        *sys = 0;
        return;
    }
    
    char comm[256];
    char state;
    int pid, ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    unsigned long utime = 0, stime = 0;
    
    if (fscanf(f, "%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu",
           &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
           &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime) != 15) {
        utime = 0;
        stime = 0;
    }
    
    *user = utime;
    *sys = stime;
    
    fclose(f);
}

static void read_meminfo(unsigned long *total, unsigned long *free, unsigned long *available)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB", total) == 1) continue;
        if (sscanf(line, "MemFree: %lu kB", free) == 1) continue;
        if (sscanf(line, "MemAvailable: %lu kB", available) == 1) continue;
    }
    
    fclose(f);
}

int main(void)
{
    unsigned long prev_user = 0, prev_sys = 0;
    int iteration = 0;
    
    printf(CLEAR_SCREEN);
    
    while (1) {
        unsigned long user, sys;
        unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
        time_t now = time(NULL);
        
        read_proc_stat(&user, &sys);
        read_meminfo(&mem_total, &mem_free, &mem_available);
        
        unsigned long user_delta = user - prev_user;
        unsigned long sys_delta = sys - prev_sys;
        
        printf("\033[H"); /* Move to top */
        
        printf("%s╔════════════════════════════════════════════════════════════════╗%s\n", COLOR_BOLD, COLOR_RESET);
        printf("%s║           Custom Syscall Performance Monitor                  ║%s\n", COLOR_BOLD, COLOR_RESET);
        printf("%s╚════════════════════════════════════════════════════════════════╝%s\n", COLOR_BOLD, COLOR_RESET);
        printf("\n");
        
        printf("%sTime:%s %s\n", COLOR_CYAN, COLOR_RESET, ctime(&now));
        printf("%sIteration:%s %d\n\n", COLOR_CYAN, COLOR_RESET, ++iteration);
        
        printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_BLUE, COLOR_RESET);
        printf("%sCPU Time (jiffies)%s\n", COLOR_BOLD, COLOR_RESET);
        printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_BLUE, COLOR_RESET);
        printf("  User time:   %s%10lu%s  (delta: %s+%lu%s)\n", 
               COLOR_GREEN, user, COLOR_RESET, COLOR_YELLOW, user_delta, COLOR_RESET);
        printf("  System time: %s%10lu%s  (delta: %s+%lu%s)\n", 
               COLOR_GREEN, sys, COLOR_RESET, COLOR_YELLOW, sys_delta, COLOR_RESET);
        
        if (user_delta + sys_delta > 0) {
            float sys_percent = (float)sys_delta / (user_delta + sys_delta) * 100.0;
            printf("  Sys/Total:   %s%.1f%%%s (syscall overhead)\n", 
                   sys_percent > 50 ? COLOR_YELLOW : COLOR_GREEN, sys_percent, COLOR_RESET);
        }
        
        printf("\n");
        printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_BLUE, COLOR_RESET);
        printf("%sMemory Status%s\n", COLOR_BOLD, COLOR_RESET);
        printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_BLUE, COLOR_RESET);
        printf("  Total:     %s%10lu kB%s (%lu MB)\n", 
               COLOR_GREEN, mem_total, COLOR_RESET, mem_total / 1024);
        printf("  Free:      %s%10lu kB%s (%lu MB)\n", 
               COLOR_GREEN, mem_free, COLOR_RESET, mem_free / 1024);
        printf("  Available: %s%10lu kB%s (%lu MB)\n", 
               COLOR_GREEN, mem_available, COLOR_RESET, mem_available / 1024);
        
        if (mem_total > 0) {
            float used_percent = (float)(mem_total - mem_available) / mem_total * 100.0;
            printf("  Used:      %s%.1f%%%s\n", 
                   used_percent > 80 ? COLOR_YELLOW : COLOR_GREEN, used_percent, COLOR_RESET);
        }
        
        printf("\n");
        printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_BLUE, COLOR_RESET);
        printf("Press Ctrl+C to exit\n");
        
        prev_user = user;
        prev_sys = sys;
        
        sleep(1);
    }
    
    return 0;
}
