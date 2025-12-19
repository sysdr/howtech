#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define REFRESH_INTERVAL_MS 500

void clear_screen() {
    printf("\033[2J\033[H");
}

void print_bar(const char* label, size_t value, size_t max_value, int width) {
    printf("%-20s ", label);
    
    int filled = (max_value > 0) ? (value * width / max_value) : 0;
    if (filled > width) filled = width;
    
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            printf("█");
        } else {
            printf("░");
        }
    }
    printf("] %8zu\n", value);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid = atoi(argv[1]);
    char maps_path[256];
    snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", target_pid);
    
    printf("\033[?25l"); // Hide cursor
    
    while (1) {
        clear_screen();
        
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║       LD_PRELOAD Memory Interposition Monitor               ║\n");
        printf("║       Tracking PID: %-10d                              ║\n", target_pid);
        printf("╚══════════════════════════════════════════════════════════════╝\n\n");
        
        // Read memory maps to show loaded libraries
        FILE* maps = fopen(maps_path, "r");
        if (maps) {
            char line[512];
            int found_preload = 0;
            int found_libc = 0;
            
            printf("Loaded Libraries:\n");
            printf("─────────────────────────────────────────────────────────────\n");
            
            while (fgets(line, sizeof(line), maps)) {
                if (strstr(line, "malloc_hook.so")) {
                    found_preload = 1;
                    char* start = strtok(line, " ");
                    printf("  \033[31m[LD_PRELOAD]\033[0m malloc_hook.so at %s\n", start);
                } else if (strstr(line, "libc-") || strstr(line, "libc.so")) {
                    if (!found_libc) {
                        found_libc = 1;
                        char* start = strtok(line, " ");
                        printf("  \033[37m[SYSTEM]\033[0m     libc.so.6 at %s\n", start);
                    }
                }
            }
            fclose(maps);
            
            if (found_preload) {
                printf("\n  \033[32m✓\033[0m Interposition active - malloc calls redirected\n");
            } else {
                printf("\n  \033[33m!\033[0m No LD_PRELOAD detected - using system malloc\n");
            }
        }
        
        printf("\n");
        printf("Press Ctrl+C to exit\n\n");
        
        // Check if process still exists
        if (access(maps_path, F_OK) != 0) {
            printf("\n\033[31mTarget process exited\033[0m\n");
            break;
        }
        
        usleep(REFRESH_INTERVAL_MS * 1000);
    }
    
    printf("\033[?25h"); // Show cursor
    return 0;
}
