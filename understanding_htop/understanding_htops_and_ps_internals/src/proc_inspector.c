#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    
    if (argc > 1) {
        pid = atoi(argv[1]);
    }
    
    printf("=== /proc Inspector ===\n\n");
    printf("Inspecting PID: %d\n\n", pid);
    
    /* Show /proc/[pid]/stat */
    char cmd[256];
    int ret;
    printf("--- /proc/%d/stat ---\n", pid);
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/stat 2>/dev/null || echo 'Process not found'", pid);
    ret = system(cmd);
    (void)ret;
    printf("\n");
    
    /* Show /proc/[pid]/status */
    printf("--- /proc/%d/status (Memory section) ---\n", pid);
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/status 2>/dev/null | grep -E '^(Vm|Rss)' || echo 'Process not found'", pid);
    ret = system(cmd);
    (void)ret;
    printf("\n");
    
    /* Show file descriptors */
    printf("--- /proc/%d/fd (File Descriptors) ---\n", pid);
    snprintf(cmd, sizeof(cmd), "ls -l /proc/%d/fd 2>/dev/null | tail -n +2 || echo 'Process not found or no permission'", pid);
    ret = system(cmd);
    (void)ret;
    printf("\n");
    
    /* Show command line */
    printf("--- /proc/%d/cmdline ---\n", pid);
    snprintf(cmd, sizeof(cmd), "cat /proc/%d/cmdline 2>/dev/null | tr '\\0' ' ' || echo 'Process not found'", pid);
    ret = system(cmd);
    (void)ret;
    printf("\n\n");
    
    return 0;
}
