/* DSQ Monitor - Real-time statistics display */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <ncurses.h>

#include "../include/scx_common.h"

static volatile sig_atomic_t stop = 0;

static void sig_handler(int sig) {
    stop = 1;
}

static void print_bar(int y, int x, const char *label, uint64_t value, uint64_t max_val) {
    int bar_width = 40;
    int filled = 0;
    
    if (max_val > 0)
        filled = (value * bar_width) / max_val;
    
    if (filled > bar_width)
        filled = bar_width;
    
    mvprintw(y, x, "%-12s [", label);
    
    attron(COLOR_PAIR(1));
    for (int i = 0; i < filled; i++)
        addch('=');
    attroff(COLOR_PAIR(1));
    
    for (int i = filled; i < bar_width; i++)
        addch(' ');
    
    printw("] %10lu", value);
}

int main(int argc, char **argv) {
    struct bpf_object *obj = NULL;
    struct bpf_map *stats_map;
    int stats_fd;
    struct dsq_stats stats = {0}, prev_stats = {0};
    uint32_t key = 0;
    int err;
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <bpf_object_path>\n", argv[0]);
        return 1;
    }
    
    /* Load BPF object */
    obj = bpf_object__open_file(argv[1], NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }
    
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }
    
    /* Find stats map */
    stats_map = bpf_object__find_map_by_name(obj, "stats_map");
    if (!stats_map) {
        fprintf(stderr, "Failed to find stats_map\n");
        goto cleanup;
    }
    
    stats_fd = bpf_map__fd(stats_map);
    if (stats_fd < 0) {
        fprintf(stderr, "Failed to get stats map FD\n");
        goto cleanup;
    }
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    start_color();
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    
    while (!stop) {
        /* Read current stats */
        if (bpf_map_lookup_elem(stats_fd, &key, &stats) != 0) {
            mvprintw(0, 0, "Failed to read stats");
            refresh();
            sleep(1);
            continue;
        }
        
        clear();
        
        /* Header */
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(0, 0, "═══════════════════════════════════════════════════════════════════════");
        mvprintw(1, 0, "        sched_ext DSQ Monitor - Priority-Based Scheduler");
        mvprintw(2, 0, "═══════════════════════════════════════════════════════════════════════");
        attroff(COLOR_PAIR(2) | A_BOLD);
        
        /* Dispatch Statistics */
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(4, 0, "DISPATCH STATISTICS (Tasks placed into DSQs)");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        uint64_t max_dispatch = stats.dispatched[0];
        if (stats.dispatched[1] > max_dispatch) max_dispatch = stats.dispatched[1];
        if (stats.dispatched[2] > max_dispatch) max_dispatch = stats.dispatched[2];
        
        print_bar(6, 2, "HIGH Prio", stats.dispatched[0], max_dispatch);
        print_bar(7, 2, "NORMAL Prio", stats.dispatched[1], max_dispatch);
        print_bar(8, 2, "LOW Prio", stats.dispatched[2], max_dispatch);
        
        /* Consume Statistics */
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(10, 0, "CONSUME STATISTICS (Tasks pulled from DSQs for execution)");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        uint64_t max_consume = stats.consumed[0];
        if (stats.consumed[1] > max_consume) max_consume = stats.consumed[1];
        if (stats.consumed[2] > max_consume) max_consume = stats.consumed[2];
        
        print_bar(12, 2, "HIGH Prio", stats.consumed[0], max_consume);
        print_bar(13, 2, "NORMAL Prio", stats.consumed[1], max_consume);
        print_bar(14, 2, "LOW Prio", stats.consumed[2], max_consume);
        
        /* DSQ Type Usage */
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(16, 0, "DSQ TYPE USAGE");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        mvprintw(18, 2, "LOCAL DSQ dispatches   : %10llu", (unsigned long long)stats.local_dispatches);
        mvprintw(19, 2, "GLOBAL DSQ dispatches  : %10llu", (unsigned long long)stats.global_dispatches);
        mvprintw(20, 2, "Custom DSQ dispatches  : %10llu", (unsigned long long)stats.custom_dispatches);
        
        /* Performance Metrics */
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(22, 0, "PERFORMANCE METRICS");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        mvprintw(24, 2, "Cache hits (same CPU)  : %10llu", (unsigned long long)stats.cache_hits);
        mvprintw(25, 2, "IPIs sent (cross-CPU)  : %10llu", (unsigned long long)stats.ipis_sent);
        
        if (stats.dispatch_errors > 0) {
            attron(COLOR_PAIR(4) | A_BOLD);
            mvprintw(26, 2, "Dispatch ERRORS        : %10llu", (unsigned long long)stats.dispatch_errors);
            attroff(COLOR_PAIR(4) | A_BOLD);
        } else {
            mvprintw(26, 2, "Dispatch ERRORS        : %10llu", (unsigned long long)stats.dispatch_errors);
        }
        
        /* Rate calculations */
        uint64_t dispatch_rate = stats.custom_dispatches - prev_stats.custom_dispatches;
        uint64_t consume_rate = (stats.consumed[0] + stats.consumed[1] + stats.consumed[2]) -
                                (prev_stats.consumed[0] + prev_stats.consumed[1] + prev_stats.consumed[2]);
        
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(28, 0, "RATES (per second)");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        mvprintw(30, 2, "Dispatch rate          : %10lu tasks/sec", dispatch_rate);
        mvprintw(31, 2, "Consume rate           : %10lu tasks/sec", consume_rate);
        
        /* Footer */
        attron(COLOR_PAIR(2));
        mvprintw(33, 0, "Press Ctrl+C to exit");
        attroff(COLOR_PAIR(2));
        
        refresh();
        
        prev_stats = stats;
        sleep(1);
    }
    
    endwin();
    printf("\nMonitor stopped.\n");
    
cleanup:
    if (obj)
        bpf_object__close(obj);
    
    return 0;
}
