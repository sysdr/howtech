#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <ncurses.h>
#include <signal.h>

#define MAX_ORDER 11
#define PAGE_SIZE 4096
#define SAMPLES 60

typedef struct {
    int node;
    char zone[32];
    unsigned long orders[MAX_ORDER];
    time_t timestamp;
} BuddyInfo;

typedef struct {
    unsigned long total_free_kb;
    unsigned long fragmentation_index;
    unsigned long largest_free_mb;
} FragStats;

static volatile int keep_running = 1;

void sigint_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

int parse_buddyinfo(BuddyInfo *info, int max_entries) {
    FILE *fp = fopen("/proc/buddyinfo", "r");
    if (!fp) {
        perror("fopen /proc/buddyinfo");
        return -1;
    }

    int count = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), fp) && count < max_entries) {
        BuddyInfo *bi = &info[count];
        
        if (sscanf(line, "Node %d, zone %31s", &bi->node, bi->zone) == 2) {
            char *ptr = line;
            // Skip to the numbers
            for (int i = 0; i < 4; i++) {
                ptr = strchr(ptr + 1, ' ');
                if (!ptr) break;
            }
            
            if (ptr) {
                for (int i = 0; i < MAX_ORDER; i++) {
                    if (sscanf(ptr, "%lu", &bi->orders[i]) == 1) {
                        ptr = strchr(ptr + 1, ' ');
                        if (!ptr && i < MAX_ORDER - 1) break;
                    } else {
                        bi->orders[i] = 0;
                    }
                }
                bi->timestamp = time(NULL);
                count++;
            }
        }
    }
    
    fclose(fp);
    return count;
}

void calculate_frag_stats(const BuddyInfo *info, int count, FragStats *stats) {
    stats->total_free_kb = 0;
    stats->largest_free_mb = 0;
    unsigned long order0_total = 0;
    unsigned long high_order_total = 0;
    
    for (int i = 0; i < count; i++) {
        for (int order = 0; order < MAX_ORDER; order++) {
            unsigned long blocks = info[i].orders[order];
            unsigned long size_kb = (PAGE_SIZE * (1UL << order)) / 1024;
            stats->total_free_kb += blocks * size_kb;
            
            if (order == 0) {
                order0_total += blocks;
            } else if (order >= 3) {
                high_order_total += blocks;
            }
            
            if (order >= 9 && blocks > 0) {
                unsigned long size_mb = (blocks * size_kb) / 1024;
                if (size_mb > stats->largest_free_mb) {
                    stats->largest_free_mb = size_mb;
                }
            }
        }
    }
    
    // Fragmentation index: ratio of order-0 to high-order blocks
    if (high_order_total > 0) {
        stats->fragmentation_index = (order0_total * 100) / (high_order_total + order0_total);
    } else {
        stats->fragmentation_index = 100;
    }
}

void draw_ui(WINDOW *win, const BuddyInfo *info, int count, const FragStats *stats) {
    werase(win);
    box(win, 0, 0);
    
    // Title
    wattron(win, A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, 1, 2, "LINUX BUDDY SYSTEM ALLOCATOR - REAL-TIME MONITOR");
    wattroff(win, A_BOLD | COLOR_PAIR(1));
    
    // Stats
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 2, 2, "Total Free: %lu MB  |  Fragmentation Index: %lu%%  |  Largest Block: %lu MB",
              stats->total_free_kb / 1024, stats->fragmentation_index, stats->largest_free_mb);
    wattroff(win, COLOR_PAIR(2));
    
    mvwprintw(win, 3, 2, "%-12s", "Zone");
    for (int order = 0; order < MAX_ORDER; order++) {
        mvwprintw(win, 3, 15 + order * 7, "O%-2d", order);
    }
    mvwprintw(win, 3, 15 + MAX_ORDER * 7, "Size");
    
    mvwhline(win, 4, 2, ACS_HLINE, 100);
    
    int row = 5;
    for (int i = 0; i < count && row < LINES - 3; i++) {
        wattron(win, COLOR_PAIR(3));
        mvwprintw(win, row, 2, "%-12s", info[i].zone);
        wattroff(win, COLOR_PAIR(3));
        
        for (int order = 0; order < MAX_ORDER; order++) {
            unsigned long blocks = info[i].orders[order];
            int color_pair = 4;
            
            if (blocks == 0) {
                color_pair = 5; // Red for depleted
            } else if (order >= 3 && blocks > 5) {
                color_pair = 6; // Green for healthy high-order
            }
            
            wattron(win, COLOR_PAIR(color_pair));
            mvwprintw(win, row, 15 + order * 7, "%5lu", blocks);
            wattroff(win, COLOR_PAIR(color_pair));
        }
        
        // Show size of order in this row
        wattron(win, COLOR_PAIR(7));
        mvwprintw(win, row, 15 + MAX_ORDER * 7, "4KB");
        wattroff(win, COLOR_PAIR(7));
        
        row++;
    }
    
    // Legend
    mvwhline(win, row, 2, ACS_HLINE, 100);
    row++;
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, row, 2, "Legend: Order N = 2^N pages = %d KB", PAGE_SIZE / 1024);
    row++;
    mvwprintw(win, row, 2, "High fragmentation = many order-0, few high-orders | Green = healthy reserves");
    wattroff(win, COLOR_PAIR(2));
    
    wrefresh(win);
}

void *allocate_test_pages(size_t size_mb) {
    size_t size = size_mb * 1024 * 1024;
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    
    // Touch pages to actually allocate them
    memset(ptr, 0, size);
    return ptr;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    signal(SIGINT, sigint_handler);
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    
    // Define color pairs
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Title
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Stats
    init_pair(3, COLOR_WHITE, COLOR_BLACK);   // Zone names
    init_pair(4, COLOR_WHITE, COLOR_BLACK);   // Normal values
    init_pair(5, COLOR_RED, COLOR_BLACK);     // Depleted
    init_pair(6, COLOR_GREEN, COLOR_BLACK);   // Healthy
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK); // Size info
    
    WINDOW *win = newwin(LINES, COLS, 0, 0);
    keypad(win, TRUE);
    nodelay(win, TRUE);
    
    BuddyInfo info[16];
    FragStats stats;
    
    void *test_alloc = NULL;
    
    while (keep_running) {
        int count = parse_buddyinfo(info, 16);
        if (count > 0) {
            calculate_frag_stats(info, count, &stats);
            draw_ui(win, info, count, &stats);
        }
        
        // Check for key press
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') {
            keep_running = 0;
        } else if (ch == 'a' || ch == 'A') {
            // Allocate test memory
            if (!test_alloc) {
                test_alloc = allocate_test_pages(100);
            }
        } else if (ch == 'f' || ch == 'F') {
            // Free test memory
            if (test_alloc) {
                munmap(test_alloc, 100 * 1024 * 1024);
                test_alloc = NULL;
            }
        }
        
        usleep(500000); // Update every 500ms
    }
    
    if (test_alloc) {
        munmap(test_alloc, 100 * 1024 * 1024);
    }
    
    delwin(win);
    endwin();
    
    printf("\nBuddy system monitoring stopped.\n");
    return 0;
}
