#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>

#define TRACE_PIPE "/sys/kernel/debug/tracing/trace_pipe"

void display_header() {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 0, "═══════════════════════════════════════════════════════════════════════════");
    mvprintw(1, 2, "eBPF LSM Security Monitor - Real-time Policy Enforcement");
    mvprintw(2, 0, "═══════════════════════════════════════════════════════════════════════════");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    attron(COLOR_PAIR(2));
    mvprintw(4, 2, "SYSTEM STATUS:");
    attroff(COLOR_PAIR(2));
    
    mvprintw(5, 4, "Kernel: %s", "5.15+");
    mvprintw(6, 4, "LSM BPF: ACTIVE");
    mvprintw(7, 4, "Monitoring: /sys/kernel/debug/tracing/trace_pipe");
    
    attron(COLOR_PAIR(2));
    mvprintw(9, 2, "RECENT POLICY VIOLATIONS:");
    attroff(COLOR_PAIR(2));
    
    refresh();
}

int main() {
    FILE *fp;
    char line[512];
    int row = 11;

    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);

    display_header();

    fp = fopen(TRACE_PIPE, "r");
    if (!fp) {
        endwin();
        fprintf(stderr, "Cannot open %s (need root)\n", TRACE_PIPE);
        fprintf(stderr, "Run with: sudo cat /sys/kernel/debug/tracing/trace_pipe\n");
        return 1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "DENIED")) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(row++, 2, "🔒 %s", line);
            attroff(COLOR_PAIR(3) | A_BOLD);
        } else if (strstr(line, "bpf_trace_printk")) {
            attron(COLOR_PAIR(4));
            mvprintw(row++, 2, "ℹ  %s", line);
            attroff(COLOR_PAIR(4));
        }

        if (row > LINES - 2) {
            row = 11;
            clear();
            display_header();
        }

        refresh();
    }

    fclose(fp);
    endwin();
    return 0;
}
