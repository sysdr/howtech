#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define CYAN "\033[0;36m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[1;33m"
#define RED "\033[0;31m"
#define BLUE "\033[0;34m"
#define BOLD "\033[1m"
#define NC "\033[0m"

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

void print_header(const char *title) {
    printf("\033[2J\033[H"); // Clear screen and move to top
    printf("%s╔══════════════════════════════════════════════════════════╗%s\n", CYAN, NC);
    printf("%s║  %-54s  ║%s\n", CYAN, title, NC);
    printf("%s╚══════════════════════════════════════════════════════════╝%s\n", CYAN, NC);
    printf("\n");
}

void read_proc_stat(pid_t pid, unsigned long *utime, unsigned long *stime) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    // Skip to the 14th and 15th fields (utime and stime)
    char dummy[256];
    if (fscanf(f, "%*d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
               dummy, utime, stime) != 3) {
        *utime = 0;
        *stime = 0;
    }
    fclose(f);
}

void monitor_process(pid_t pid) {
    signal(SIGINT, sigint_handler);
    
    unsigned long prev_utime = 0, prev_stime = 0;
    unsigned long curr_utime = 0, curr_stime = 0;
    int sample = 0;
    
    read_proc_stat(pid, &prev_utime, &prev_stime);
    
    while (keep_running) {
        usleep(500000); // 500ms
        
        read_proc_stat(pid, &curr_utime, &curr_stime);
        
        unsigned long delta_utime = curr_utime - prev_utime;
        unsigned long delta_stime = curr_stime - prev_stime;
        unsigned long total_delta = delta_utime + delta_stime;
        
        print_header("Real-Time Process Monitor");
        
        printf("%sPID: %d%s                                Sample: %d\n", 
               BOLD, pid, NC, ++sample);
        printf("──────────────────────────────────────────────────────────\n\n");
        
        printf("%sCPU Time Distribution:%s\n", BOLD, NC);
        if (total_delta > 0) {
            int user_pct = (delta_utime * 100) / total_delta;
            int sys_pct = (delta_stime * 100) / total_delta;
            
            printf("  User:   [");
            for (int i = 0; i < 40; i++) {
                if (i < user_pct * 40 / 100) printf("%s█%s", GREEN, NC);
                else printf(" ");
            }
            printf("] %3d%%\n", user_pct);
            
            printf("  System: [");
            for (int i = 0; i < 40; i++) {
                if (i < sys_pct * 40 / 100) printf("%s█%s", YELLOW, NC);
                else printf(" ");
            }
            printf("] %3d%%\n", sys_pct);
        } else {
            printf("  %sNo CPU activity detected%s\n", YELLOW, NC);
        }
        
        printf("\n%sCPU Jiffies (since start):%s\n", BOLD, NC);
        printf("  User time:   %lu jiffies\n", curr_utime);
        printf("  System time: %lu jiffies\n", curr_stime);
        printf("  Total:       %lu jiffies\n", curr_utime + curr_stime);
        
        printf("\n%sProcess Status:%s ", BOLD, NC);
        char status_path[256];
        snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
        FILE *f = fopen(status_path, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "State:", 6) == 0) {
                    printf("%s%s%s", GREEN, line + 7, NC);
                    break;
                }
            }
            fclose(f);
        } else {
            printf("%sProcess exited%s\n", RED, NC);
            break;
        }
        
        printf("\n%sPress Ctrl+C to stop monitoring%s\n", BLUE, NC);
        
        prev_utime = curr_utime;
        prev_stime = curr_stime;
        
        // Check if process still exists
        if (kill(pid, 0) != 0) {
            printf("\n%sProcess %d has exited%s\n", YELLOW, pid, NC);
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    monitor_process(pid);
    
    return 0;
}
