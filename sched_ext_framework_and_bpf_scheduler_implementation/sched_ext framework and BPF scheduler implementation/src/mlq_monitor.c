// SPDX-License-Identifier: GPL-2.0
/* Real-time ncurses-based monitor for multi-level scheduler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h>
#include <bpf/bpf.h>
#include "../include/multi_level_sched.h"

#define UPDATE_INTERVAL_MS 500
#define HISTORY_SIZE 60

static volatile sig_atomic_t running = 1;

struct queue_history {
    unsigned long long depth[HISTORY_SIZE];
    int index;
};

static void sig_handler(int sig)
{
    running = 0;
}

static void draw_sparkline(WINDOW *win, int y, int x, unsigned long long *data, 
                          int len, unsigned long long max_val, int width)
{
    const char *bars[] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    
    for (int i = 0; i < width && i < len; i++) {
        unsigned long long val = data[(len - width + i) % len];
        int bar_idx = max_val > 0 ? (val * 8 / max_val) : 0;
        if (bar_idx > 8) bar_idx = 8;
        mvwprintw(win, y, x + i, "%s", bars[bar_idx]);
    }
}

static unsigned long long get_max_value(unsigned long long *data, int len)
{
    unsigned long long max = 1;
    for (int i = 0; i < len; i++) {
        if (data[i] > max)
            max = data[i];
    }
    return max;
}

int main(int argc, char **argv)
{
    int stats_fd;
    struct sched_stats stats, prev_stats = {0};
    struct queue_history history[PRIORITY_MAX] = {0};
    WINDOW *win;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <stats_map_fd>\n", argv[0]);
        return 1;
    }
    
    stats_fd = atoi(argv[1]);
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  /* HIGH priority */
        init_pair(2, COLOR_YELLOW, COLOR_BLACK); /* MEDIUM priority */
        init_pair(3, COLOR_BLUE, COLOR_BLACK);   /* LOW priority */
        init_pair(4, COLOR_RED, COLOR_BLACK);    /* Headers */
    }
    
    win = newwin(24, 80, 0, 0);
    keypad(win, TRUE);
    nodelay(win, TRUE);
    
    time_t start_time = time(NULL);
    
    while (running) {
        __u32 key = 0;
        
        if (bpf_map_lookup_elem(stats_fd, &key, &stats) != 0) {
            mvwprintw(win, 0, 0, "Failed to read stats");
            wrefresh(win);
            usleep(UPDATE_INTERVAL_MS * 1000);
            continue;
        }
        
        werase(win);
        box(win, 0, 0);
        
        /* Header */
        wattron(win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(win, 1, 2, "Multi-Level FIFO Scheduler Monitor");
        wattroff(win, COLOR_PAIR(4) | A_BOLD);
        
        time_t elapsed = time(NULL) - start_time;
        mvwprintw(win, 1, 55, "Uptime: %02ld:%02ld:%02ld", 
                  elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
        
        mvwhline(win, 2, 1, ACS_HLINE, 78);
        
        /* Queue statistics */
        const char *level_names[] = {"HIGH", "MEDIUM", "LOW"};
        int colors[] = {1, 2, 3};
        
        for (int i = 0; i < PRIORITY_MAX; i++) {
            int y = 4 + i * 5;
            
            wattron(win, COLOR_PAIR(colors[i]) | A_BOLD);
            mvwprintw(win, y, 2, "%s Priority Queue:", level_names[i]);
            wattroff(win, COLOR_PAIR(colors[i]) | A_BOLD);
            
            mvwprintw(win, y + 1, 4, "Enqueued:   %12llu  (+%llu/sec)", 
                      stats.enqueued[i],
                      stats.enqueued[i] - prev_stats.enqueued[i]);
            
            mvwprintw(win, y + 2, 4, "Dispatched: %12llu  (+%llu/sec)",
                      stats.dispatched[i],
                      stats.dispatched[i] - prev_stats.dispatched[i]);
            
            mvwprintw(win, y + 3, 4, "Queue Depth:  %10llu  ", stats.current_depth[i]);
            
            /* Update history */
            history[i].depth[history[i].index] = stats.current_depth[i];
            history[i].index = (history[i].index + 1) % HISTORY_SIZE;
            
            /* Draw sparkline */
            wattron(win, COLOR_PAIR(colors[i]));
            unsigned long long max = get_max_value(history[i].depth, HISTORY_SIZE);
            draw_sparkline(win, y + 3, 35, history[i].depth, HISTORY_SIZE, max, 40);
            wattroff(win, COLOR_PAIR(colors[i]));
        }
        
        /* Instructions */
        mvwhline(win, 20, 1, ACS_HLINE, 78);
        mvwprintw(win, 21, 2, "Press 'q' or Ctrl+C to exit");
        mvwprintw(win, 22, 2, "Update interval: %dms", UPDATE_INTERVAL_MS);
        
        wrefresh(win);
        
        /* Check for quit */
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q')
            break;
        
        prev_stats = stats;
        usleep(UPDATE_INTERVAL_MS * 1000);
    }
    
    delwin(win);
    endwin();
    
    printf("\nMonitor terminated.\n");
    return 0;
}
