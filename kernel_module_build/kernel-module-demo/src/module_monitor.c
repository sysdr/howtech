/*
 * module_monitor.c - Real-time kernel module status monitor
 * Displays module state, memory usage, and kernel logs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define MODULE_NAME "hello_module"
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RED "\033[0;31m"
#define COLOR_CYAN "\033[0;36m"
#define BOLD "\033[1m"

void clear_screen() {
    printf("\033[2J\033[H");
}

void print_header() {
    time_t now = time(NULL);
    printf("%s%s╔══════════════════════════════════════════════════════════════════════╗%s\n", 
           BOLD, COLOR_BLUE, COLOR_RESET);
    printf("%s%s║     KERNEL MODULE STATUS MONITOR - %s", 
           BOLD, COLOR_BLUE, MODULE_NAME);
    printf("%*s║%s\n", (int)(28 - strlen(MODULE_NAME)), "", COLOR_RESET);
    printf("%s%s║     %s", BOLD, COLOR_BLUE, ctime(&now));
    printf("                                           ║%s\n", COLOR_RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════════════╝%s\n\n", 
           BOLD, COLOR_BLUE, COLOR_RESET);
}

int check_module_loaded() {
    FILE *fp = fopen("/proc/modules", "r");
    if (!fp) return 0;
    
    char line[256];
    int loaded = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, MODULE_NAME, strlen(MODULE_NAME)) == 0) {
            loaded = 1;
            break;
        }
    }
    fclose(fp);
    return loaded;
}

void print_module_info() {
    printf("%s%s┌─ MODULE STATE ────────────────────────────────────────────────────┐%s\n", 
           BOLD, COLOR_GREEN, COLOR_RESET);
    
    int loaded = check_module_loaded();
    printf("│  Status: %s%s%-12s%s", 
           loaded ? COLOR_GREEN : COLOR_RED,
           loaded ? "●" : "○",
           loaded ? "LOADED" : "NOT LOADED",
           COLOR_RESET);
    printf("                                           │\n");
    
    if (loaded) {
        // Read /proc/modules
        FILE *fp = fopen("/proc/modules", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, MODULE_NAME, strlen(MODULE_NAME)) == 0) {
                    int size, refcnt;
                    char deps[64], state[16];
                    unsigned long addr;
                    sscanf(line, "%*s %d %d %s %s %lx", &size, &refcnt, deps, state, &addr);
                    
                    printf("│  Size: %s%d bytes%s", COLOR_CYAN, size, COLOR_RESET);
                    printf("%*s│\n", 54 - snprintf(NULL, 0, "%d bytes", size), "");
                    
                    printf("│  Refcount: %s%d%s", COLOR_CYAN, refcnt, COLOR_RESET);
                    printf("%*s│\n", 58 - snprintf(NULL, 0, "%d", refcnt), "");
                    
                    printf("│  State: %s%s%s", COLOR_CYAN, state, COLOR_RESET);
                    printf("%*s│\n", (int)(61 - strlen(state)), "");
                    
                    printf("│  Address: %s0x%lx%s", COLOR_CYAN, addr, COLOR_RESET);
                    printf("%*s│\n", 53 - snprintf(NULL, 0, "0x%lx", addr), "");
                    break;
                }
            }
            fclose(fp);
        }
        
        // Read parameters if sysfs available
        char param_path[256];
        snprintf(param_path, sizeof(param_path), "/sys/module/%s/parameters/name", MODULE_NAME);
        fp = fopen(param_path, "r");
        if (fp) {
            char param[64];
            if (fgets(param, sizeof(param), fp)) {
                param[strcspn(param, "\n")] = 0;
                printf("│  Parameter 'name': %s%s%s", COLOR_YELLOW, param, COLOR_RESET);
                printf("%*s│\n", (int)(47 - strlen(param)), "");
            }
            fclose(fp);
        }
        
        snprintf(param_path, sizeof(param_path), "/sys/module/%s/parameters/count", MODULE_NAME);
        fp = fopen(param_path, "r");
        if (fp) {
            char param[64];
            if (fgets(param, sizeof(param), fp)) {
                param[strcspn(param, "\n")] = 0;
                printf("│  Parameter 'count': %s%s%s", COLOR_YELLOW, param, COLOR_RESET);
                printf("%*s│\n", (int)(46 - strlen(param)), "");
            }
            fclose(fp);
        }
    }
    
    printf("%s%s└───────────────────────────────────────────────────────────────────┘%s\n\n", 
           BOLD, COLOR_GREEN, COLOR_RESET);
}

void print_recent_logs() {
    printf("%s%s┌─ KERNEL LOGS (last 10 entries) ───────────────────────────────────┐%s\n", 
           BOLD, COLOR_YELLOW, COLOR_RESET);
    
    FILE *fp = popen("dmesg | grep hello_module | tail -10", "r");
    if (fp) {
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), fp) && count < 10) {
            // Trim newline
            line[strcspn(line, "\n")] = 0;
            
            // Truncate if too long
            if (strlen(line) > 67) {
                line[64] = '.';
                line[65] = '.';
                line[66] = '.';
                line[67] = 0;
            }
            
            printf("│  %s%-67s%s│\n", COLOR_RESET, line, COLOR_RESET);
            count++;
        }
        
        if (count == 0) {
            printf("│  %sNo kernel logs found for %s%s%*s│\n", 
                   COLOR_RED, MODULE_NAME, COLOR_RESET, 32, "");
        }
        
        pclose(fp);
    }
    
    printf("%s%s└───────────────────────────────────────────────────────────────────┘%s\n\n", 
           BOLD, COLOR_YELLOW, COLOR_RESET);
}

void print_instructions() {
    printf("%s%sINSTRUCTIONS:%s\n", BOLD, COLOR_CYAN, COLOR_RESET);
    printf("  • This monitor updates every 2 seconds\n");
    printf("  • Load module: sudo insmod hello_module.ko\n");
    printf("  • With params: sudo insmod hello_module.ko name=Alice count=3\n");
    printf("  • Unload: sudo rmmod hello_module\n");
    printf("  • Press Ctrl+C to exit monitor\n\n");
}

int main() {
    printf("Starting module monitor...\n");
    sleep(1);
    
    while (1) {
        clear_screen();
        print_header();
        print_module_info();
        print_recent_logs();
        print_instructions();
        
        printf("%sRefreshing in 2 seconds...%s\n", COLOR_BLUE, COLOR_RESET);
        sleep(2);
    }
    
    return 0;
}
