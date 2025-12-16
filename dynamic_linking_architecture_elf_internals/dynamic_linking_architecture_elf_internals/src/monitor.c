#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <link.h>
#include <dlfcn.h>
#include <ncurses.h>
#include <time.h>
#include <stdint.h>

extern void example_function(const char *msg);
extern void another_function(int value);

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

typedef struct {
    const char *name;
    void *func_ptr;
    void *got_entry;
    uint64_t first_call_cycles;
    uint64_t avg_cycles;
    int call_count;
} FunctionInfo;

void display_header(WINDOW *win) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, 0, 0, "╔════════════════════════════════════════════════════════════════════════════╗");
    mvwprintw(win, 1, 0, "║         GOT/PLT Runtime Monitor - Dynamic Symbol Resolution            ║");
    mvwprintw(win, 2, 0, "╚════════════════════════════════════════════════════════════════════════════╝");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
}

void display_function_info(WINDOW *win, FunctionInfo *funcs, int count, int y_offset) {
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, y_offset++, 2, "Symbol Resolution Status:");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    
    mvwprintw(win, y_offset++, 2, "%-20s %-18s %-10s %-15s", 
              "Function", "Address", "Calls", "Avg Cycles");
    mvwprintw(win, y_offset++, 2, "────────────────────────────────────────────────────────────────────");
    
    for (int i = 0; i < count; i++) {
        if (funcs[i].call_count == 0) {
            wattron(win, COLOR_PAIR(4)); // Yellow for unresolved
            mvwprintw(win, y_offset++, 2, "%-20s %-18p %-10s %-15s",
                      funcs[i].name, funcs[i].func_ptr, "UNRESOLVED", "---");
            wattroff(win, COLOR_PAIR(4));
        } else if (funcs[i].call_count == 1) {
            wattron(win, COLOR_PAIR(3)); // Red for first call
            mvwprintw(win, y_offset++, 2, "%-20s %-18p %-10d %lu (RESOLVER)",
                      funcs[i].name, funcs[i].func_ptr, 
                      funcs[i].call_count, funcs[i].first_call_cycles);
            wattroff(win, COLOR_PAIR(3));
        } else {
            wattron(win, COLOR_PAIR(5)); // Green for resolved
            mvwprintw(win, y_offset++, 2, "%-20s %-18p %-10d %lu",
                      funcs[i].name, funcs[i].func_ptr, 
                      funcs[i].call_count, funcs[i].avg_cycles);
            wattroff(win, COLOR_PAIR(5));
        }
    }
}

void display_memory_map(WINDOW *win, int y_offset) {
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, y_offset++, 2, "Memory Layout (ASLR):");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), fp) && count < 6) {
            if (strstr(line, "[stack]") || strstr(line, "[heap]") || 
                strstr(line, "libexample") || strstr(line, "monitor")) {
                line[strcspn(line, "\n")] = 0;
                char addr[32], perms[8], rest[200];
                if (sscanf(line, "%s %s %*s %*s %*s %[^\n]", addr, perms, rest) >= 2) {
                    mvwprintw(win, y_offset++, 2, "  %s %s %s", addr, perms, rest);
                    count++;
                }
            }
        }
        fclose(fp);
    }
}

int main(void) {
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100);
    
    // Color pairs
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Header
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Section titles
    init_pair(3, COLOR_RED, COLOR_BLACK);     // First call
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Unresolved
    init_pair(5, COLOR_GREEN, COLOR_BLACK);   // Resolved
    
    WINDOW *win = newwin(LINES, COLS, 0, 0);
    keypad(win, TRUE);
    
    FunctionInfo funcs[] = {
        {"example_function", (void*)example_function, NULL, 0, 0, 0},
        {"another_function", (void*)another_function, NULL, 0, 0, 0},
        {"printf", (void*)printf, NULL, 0, 0, 0},
    };
    int func_count = sizeof(funcs) / sizeof(funcs[0]);
    
    int running = 1;
    int frame = 0;
    
    while (running) {
        werase(win);
        
        display_header(win);
        
        // Status
        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, 4, 2, "Frame: %d | Press 'q' to quit, 'c' to call functions", frame++);
        wattroff(win, COLOR_PAIR(2));
        
        // Display function info
        display_function_info(win, funcs, func_count, 6);
        
        // Display memory map
        display_memory_map(win, 15);
        
        // Instructions
        wattron(win, COLOR_PAIR(4));
        mvwprintw(win, LINES-3, 2, "Legend:");
        mvwprintw(win, LINES-2, 2, "  RED: First call (resolver) | GREEN: Resolved | YELLOW: Not yet called");
        wattroff(win, COLOR_PAIR(4));
        
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') {
            running = 0;
        } else if (ch == 'c' || ch == 'C') {
            // Call functions and measure
            for (int i = 0; i < func_count; i++) {
                uint64_t start = rdtsc();
                
                if (i == 0) {
                    example_function("test");
                } else if (i == 1) {
                    another_function(42);
                } else if (i == 2) {
                    printf("test\n");
                }
                
                uint64_t end = rdtsc();
                uint64_t cycles = end - start;
                
                if (funcs[i].call_count == 0) {
                    funcs[i].first_call_cycles = cycles;
                }
                funcs[i].call_count++;
                
                // Update running average
                if (funcs[i].call_count > 1) {
                    funcs[i].avg_cycles = (funcs[i].avg_cycles * (funcs[i].call_count - 1) + cycles) / funcs[i].call_count;
                }
            }
        }
        
        usleep(50000); // 50ms
    }
    
    delwin(win);
    endwin();
    
    printf("\n=== Final Statistics ===\n");
    for (int i = 0; i < func_count; i++) {
        printf("%s: %d calls, first: %lu cycles, avg: %lu cycles\n",
               funcs[i].name, funcs[i].call_count, 
               funcs[i].first_call_cycles, funcs[i].avg_cycles);
    }
    
    return 0;
}
