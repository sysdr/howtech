#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define BOLD "\033[1m"
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"

void read_proc_stat(unsigned long long *user, unsigned long long *system, unsigned long long *idle) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("fopen /proc/stat");
        exit(EXIT_FAILURE);
    }
    
    unsigned long long nice;
    if (fscanf(fp, "cpu  %llu %llu %llu %llu", user, &nice, system, idle) != 4) {
        perror("fscanf /proc/stat");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    (void)nice;  // Discard nice value
    fclose(fp);
}

void read_proc_interrupts(unsigned long long *total_irqs) {
    FILE *fp = fopen("/proc/interrupts", "r");
    if (!fp) return;
    
    char line[1024];
    *total_irqs = 0;
    
    // Skip header
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        unsigned long long irq_count;
        if (sscanf(line, "%*s %llu", &irq_count) == 1) {
            *total_irqs += irq_count;
        }
    }
    
    fclose(fp);
}

int main(void) {
    unsigned long long prev_user = 0, prev_system = 0, prev_idle = 0;
    unsigned long long prev_irqs = 0;
    
    read_proc_stat(&prev_user, &prev_system, &prev_idle);
    read_proc_interrupts(&prev_irqs);
    
    printf(CLEAR_SCREEN);
    printf("%s%s╔════════════════════════════════════════════════════════════════╗%s\n", BOLD, BLUE, RESET);
    printf("%s%s║          Live Lock Detection Monitor (5s intervals)          ║%s\n", BOLD, BLUE, RESET);
    printf("%s%s╚════════════════════════════════════════════════════════════════╝%s\n\n", BOLD, BLUE, RESET);
    
    while (1) {
        sleep(5);
        
        unsigned long long user, system, idle, irqs;
        read_proc_stat(&user, &system, &idle);
        read_proc_interrupts(&irqs);
        
        unsigned long long user_delta = user - prev_user;
        unsigned long long sys_delta = system - prev_system;
        unsigned long long idle_delta = idle - prev_idle;
        unsigned long long total_delta = user_delta + sys_delta + idle_delta;
        
        unsigned long long irq_delta = irqs - prev_irqs;
        
        double user_pct = 100.0 * user_delta / total_delta;
        double sys_pct = 100.0 * sys_delta / total_delta;
        double idle_pct = 100.0 * idle_delta / total_delta;
        
        printf("\r%s[%s] CPU Usage:%s  User: %s%5.1f%%%s  Sys: %s%5.1f%%%s  Idle: %5.1f%%  |  IRQs/sec: %s%llu%s",
               BOLD, __TIME__, RESET,
               GREEN, user_pct, RESET,
               (sys_pct > 50) ? RED : YELLOW, sys_pct, RESET,
               idle_pct,
               (irq_delta > 100000) ? RED : GREEN, irq_delta / 5, RESET);
        
        fflush(stdout);
        
        if (sys_pct > 80) {
            printf("  %s⚠ HIGH SYSTEM TIME - Possible live lock!%s", RED, RESET);
        }
        
        prev_user = user;
        prev_system = system;
        prev_idle = idle;
        prev_irqs = irqs;
    }
    
    return 0;
}
