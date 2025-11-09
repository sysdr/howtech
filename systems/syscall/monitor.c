#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define COLOR_CYAN    "\x1b[36m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET   "\x1b[0m"

int main(void) {
    printf(COLOR_CYAN "\n╔═══════════════════════════════════════════════════╗\n");
    printf("║         Real-time Syscall Monitor                ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n" COLOR_RESET);
    
    printf("\nRun in another terminal:\n");
    printf(COLOR_YELLOW "  strace -c -p $(pgrep syscall_demo)\n" COLOR_RESET);
    printf(COLOR_YELLOW "  perf trace -p $(pgrep syscall_demo)\n" COLOR_RESET);
    
    printf("\nMonitoring syscalls via /proc...\n\n");
    
    printf(COLOR_GREEN "PID\t\tSyscall\t\tArgs\n" COLOR_RESET);
    printf("────────────────────────────────────────────────\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep syscall_demo");
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char pid[16];
        while (fgets(pid, sizeof(pid), fp)) {
            pid[strcspn(pid, "\n")] = 0;
            char syscall_path[256];
            snprintf(syscall_path, sizeof(syscall_path), "/proc/%s/syscall", pid);
            
            FILE *sf = fopen(syscall_path, "r");
            if (sf) {
                char line[256];
                if (fgets(line, sizeof(line), sf)) {
                    printf("%s\t%s", pid, line);
                }
                fclose(sf);
            }
        }
        pclose(fp);
    }
    
    printf("\n" COLOR_CYAN "Note: " COLOR_RESET "This is a simple monitor.\n");
    printf("For real-time tracing, use: " COLOR_GREEN "perf trace" COLOR_RESET " or " COLOR_GREEN "strace" COLOR_RESET "\n\n");
    
    return 0;
}
