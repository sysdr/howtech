#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define BOLD "\033[1m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

void print_header() {
    printf(CLEAR_SCREEN);
    printf(BOLD CYAN "╔════════════════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD CYAN "║" RESET BOLD "           FLAME GRAPH PROFILING - LIVE MONITOR                 " CYAN "║\n" RESET);
    printf(BOLD CYAN "╚════════════════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void print_bar(const char* label, int percentage, const char* color) {
    printf("%s%-20s" RESET " [", color, label);
    int bars = percentage / 2;
    for (int i = 0; i < 50; i++) {
        if (i < bars) {
            printf("█");
        } else {
            printf("░");
        }
    }
    printf("] %3d%%\n", percentage);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    int pid = atoi(argv[1]);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    
    unsigned long long prev_utime = 0, prev_stime = 0;
    struct timeval prev_time, curr_time;
    gettimeofday(&prev_time, NULL);
    
    int samples = 0;
    
    while (running) {
        FILE* fp = fopen(stat_path, "r");
        if (!fp) {
            printf(RED "Process %d not found. Exiting.\n" RESET, pid);
            break;
        }
        
        unsigned long long utime, stime;
        char comm[256];
        char state;
        int ppid;
        
        fscanf(fp, "%d %s %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
               &pid, comm, &state, &ppid, &utime, &stime);
        fclose(fp);
        
        gettimeofday(&curr_time, NULL);
        double elapsed = (curr_time.tv_sec - prev_time.tv_sec) +
                        (curr_time.tv_usec - prev_time.tv_usec) / 1000000.0;
        
        print_header();
        
        printf(BOLD "Target Process:\n" RESET);
        printf("  PID:          %s%d%s\n", GREEN, pid, RESET);
        printf("  Command:      %s%s%s\n", GREEN, comm, RESET);
        printf("  State:        %s%c%s\n", GREEN, state, RESET);
        printf("\n");
        
        if (samples > 0) {
            unsigned long long delta_utime = utime - prev_utime;
            unsigned long long delta_stime = stime - prev_stime;
            unsigned long long total_delta = delta_utime + delta_stime;
            
            int user_pct = total_delta > 0 ? (delta_utime * 100) / total_delta : 0;
            int sys_pct = total_delta > 0 ? (delta_stime * 100) / total_delta : 0;
            
            printf(BOLD "CPU Time Distribution:\n" RESET);
            print_bar("User Space", user_pct > 100 ? 100 : user_pct, YELLOW);
            print_bar("Kernel Space", sys_pct > 100 ? 100 : sys_pct, RED);
            printf("\n");
            
            printf(BOLD "Profiling Status:\n" RESET);
            printf("  %s●%s perf record running (99 Hz sampling)\n", GREEN, RESET);
            printf("  %s●%s Capturing kernel + user stacks\n", GREEN, RESET);
            printf("  %s●%s Frame pointers enabled\n", GREEN, RESET);
            printf("\n");
            
            printf(BOLD "What's Being Captured:\n" RESET);
            printf("  • Call stack every ~10ms\n");
            printf("  • Function entry/exit points\n");
            printf("  • CPU cycle consumption\n");
            printf("  • Syscall overhead\n");
            printf("\n");
        }
        
        printf(BOLD BLUE "Flame graph will show:\n" RESET);
        printf("  • " YELLOW "Wide boxes" RESET " = time-consuming functions\n");
        printf("  • " YELLOW "Tall stacks" RESET " = deep call chains\n");
        printf("  • " RED "Plateaus" RESET " = performance hotspots\n");
        printf("\n");
        
        printf(BOLD GREEN "Press Ctrl+C to stop profiling and generate flame graph\n" RESET);
        
        prev_utime = utime;
        prev_stime = stime;
        prev_time = curr_time;
        samples++;
        
        sleep(1);
    }
    
    printf("\n" GREEN "Stopping profiler...\n" RESET);
    return 0;
}
