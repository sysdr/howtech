#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_memory_info(int pid) {
    char maps_path[256], status_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
    
    printf("\n=== Memory Mappings (/proc/%d/maps) ===\n", pid);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cat %s 2>/dev/null | head -20", maps_path);
    system(cmd);
    
    printf("\n=== Memory Status (/proc/%d/status) ===\n", pid);
    snprintf(cmd, sizeof(cmd), 
             "cat %s 2>/dev/null | grep -E 'VmSize|VmRSS|VmData|VmStk|VmExe'", 
             status_path);
    system(cmd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    int pid = atoi(argv[1]);
    print_memory_info(pid);
    
    return 0;
}
