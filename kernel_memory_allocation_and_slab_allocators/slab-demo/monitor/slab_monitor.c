// slab_monitor.c - Real-time slab allocator monitor
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

#define MAX_CACHES 50
#define REFRESH_RATE 1

struct cache_info {
    char name[64];
    unsigned long active_objs;
    unsigned long num_objs;
    unsigned long objsize;
    unsigned long objperslab;
    unsigned long active_slabs;
    unsigned long num_slabs;
};

int parse_slabinfo(struct cache_info *caches, int max_caches) {
    FILE *f = fopen("/proc/slabinfo", "r");
    if (!f) {
        return -1;
    }
    
    char line[512];
    int count = 0;
    
    // Skip header lines
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return 0;
    }
    if (fgets(line, sizeof(line), f) == NULL) {
        fclose(f);
        return 0;
    }
    
    while (fgets(line, sizeof(line), f) && count < max_caches) {
        // Focus on interesting caches
        if (strstr(line, "kmalloc") || 
            strstr(line, "task_struct") ||
            strstr(line, "dentry") ||
            strstr(line, "inode") ||
            strstr(line, "buffer_head") ||
            strstr(line, "ext4") ||
            strstr(line, "slab_demo")) {
            
            struct cache_info *c = &caches[count];
            sscanf(line, "%s %lu %lu %lu %lu : tunables %*d %*d %*d : slabdata %lu %lu",
                   c->name, &c->active_objs, &c->num_objs, &c->objsize,
                   &c->objperslab, &c->active_slabs, &c->num_slabs);
            count++;
        }
    }
    
    fclose(f);
    return count;
}

void display_header(void) {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 0, "╔═══════════════════════════════════════════════════════════════════════════════╗");
    mvprintw(1, 0, "║         SLAB ALLOCATOR REAL-TIME MONITOR - Systems Programming Deep Dive     ║");
    mvprintw(2, 0, "╚═══════════════════════════════════════════════════════════════════════════════╝");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(2));
    mvprintw(4, 2, "Cache Name");
    mvprintw(4, 25, "Active");
    mvprintw(4, 35, "Total");
    mvprintw(4, 45, "ObjSize");
    mvprintw(4, 56, "PerSlab");
    mvprintw(4, 66, "Slabs");
    mvprintw(4, 75, "Usage%%");
    attroff(COLOR_PAIR(2));
    
    mvprintw(5, 0, "──────────────────────────────────────────────────────────────────────────────────");
}

void display_cache(int row, struct cache_info *cache) {
    int usage = cache->num_objs > 0 ? 
                (cache->active_objs * 100) / cache->num_objs : 0;
    
    int color = 3;  // Default green
    if (usage > 80) color = 4;  // Yellow
    if (usage > 95) color = 5;  // Red
    
    attron(COLOR_PAIR(color));
    mvprintw(row, 2, "%-20s", cache->name);
    mvprintw(row, 25, "%8lu", cache->active_objs);
    mvprintw(row, 35, "%8lu", cache->num_objs);
    mvprintw(row, 45, "%8lu", cache->objsize);
    mvprintw(row, 56, "%8lu", cache->objperslab);
    mvprintw(row, 66, "%4lu/%lu", cache->active_slabs, cache->num_slabs);
    mvprintw(row, 75, "%3d%%", usage);
    attroff(COLOR_PAIR(color));
}

void display_footer(int row) {
    attron(COLOR_PAIR(2));
    mvprintw(row + 2, 0, "──────────────────────────────────────────────────────────────────────────────────");
    mvprintw(row + 3, 2, "SLUB Fast Path: Per-CPU freelists (lock-free)");
    mvprintw(row + 4, 2, "SLUB Slow Path: Refill from partial lists (lock required)");
    mvprintw(row + 5, 2, "High usage (>95%%) = potential memory pressure");
    mvprintw(row + 7, 2, "Press 'q' to quit | Refresh: every %d second(s)", REFRESH_RATE);
    attroff(COLOR_PAIR(2));
}

int main(void) {
    struct cache_info caches[MAX_CACHES];
    int num_caches;
    
    // Check if we can read slabinfo
    FILE *test = fopen("/proc/slabinfo", "r");
    if (!test) {
        printf("Error: Cannot read /proc/slabinfo\n");
        printf("Try running with: sudo ./build/slab_monitor\n");
        return 1;
    }
    fclose(test);
    
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    // Initialize colors
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Header
    init_pair(2, COLOR_WHITE, COLOR_BLACK);   // Normal
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Low usage
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Medium usage
    init_pair(5, COLOR_RED, COLOR_BLACK);     // High usage
    
    while (1) {
        clear();
        
        // Parse current slab info
        num_caches = parse_slabinfo(caches, MAX_CACHES);
        
        if (num_caches < 0) {
            endwin();
            printf("Error reading /proc/slabinfo\n");
            return 1;
        }
        
        // Display interface
        display_header();
        
        for (int i = 0; i < num_caches && i < 20; i++) {
            display_cache(6 + i, &caches[i]);
        }
        
        display_footer(6 + num_caches);
        
        refresh();
        
        // Check for quit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        
        sleep(REFRESH_RATE);
    }
    
    endwin();
    return 0;
}
