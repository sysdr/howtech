#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define CORES_DIR "./cores"

void clear_screen() {
    printf("\033[2J\033[H");
}

void print_header() {
    printf("\033[1;36m");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║          Core Dump Monitoring Dashboard                       ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\033[0m\n");
    printf("\n");
}

void print_core_info(const char *filename, struct stat *st) {
    printf("\033[1;33m📁 Core File:\033[0m %s\n", filename);
    printf("   \033[1;32m├─\033[0m Size: %.2f MB (%ld bytes)\n", 
           st->st_size / (1024.0 * 1024.0), st->st_size);
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", 
             localtime(&st->st_mtime));
    printf("   \033[1;32m├─\033[0m Modified: %s\n", time_str);
    printf("   \033[1;32m└─\033[0m Permissions: %o\n\n", st->st_mode & 0777);
}

void check_core_pattern() {
    printf("\033[1;34m🔧 System Configuration:\033[0m\n");
    
    FILE *f = fopen("/proc/sys/kernel/core_pattern", "r");
    if (f) {
        char pattern[256];
        if (fgets(pattern, sizeof(pattern), f)) {
            pattern[strcspn(pattern, "\n")] = 0;
            printf("   Core pattern: %s\n", pattern);
            if (pattern[0] == '|') {
                printf("   \033[1;33m⚠️  Using pipe handler (systemd-coredump?)\033[0m\n");
            }
        }
        fclose(f);
    }
    printf("\n");
}

void monitor_cores() {
    DIR *dir = opendir(CORES_DIR);
    if (!dir) {
        printf("\033[1;31m❌ Error: Could not open %s\033[0m\n", CORES_DIR);
        printf("   Core directory doesn't exist or no permission.\n");
        return;
    }
    
    int core_count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "core", 4) == 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", CORES_DIR, entry->d_name);
            
            struct stat st;
            if (stat(path, &st) == 0) {
                print_core_info(entry->d_name, &st);
                core_count++;
            }
        }
    }
    
    closedir(dir);
    
    if (core_count == 0) {
        printf("\033[1;33m⏳ No core files found yet...\033[0m\n");
        printf("   Waiting for crash...\n\n");
    } else {
        printf("\033[1;32m✓ Total cores found: %d\033[0m\n\n", core_count);
    }
}

int main() {
    clear_screen();
    print_header();
    check_core_pattern();
    monitor_cores();
    
    printf("\033[1;36m");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\033[0m");
    printf("Press Ctrl+C to exit\n");
    
    return 0;
}
