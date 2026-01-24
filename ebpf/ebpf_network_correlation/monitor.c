#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#define MAX_ENTRIES 1024

struct conn_info {
    __u32 pid;
    __u32 tgid;
    char comm[16];
    __u64 timestamp;
    __u32 saddr;
    __u32 daddr;
    __u16 sport;
    __u16 dport;
};

static void draw_header(WINDOW *win) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, 0, 0, "╔══════════════════════════════════════════════════════════════════════════════╗");
    mvwprintw(win, 1, 0, "║");
    mvwprintw(win, 1, 78, "║");
    mvwprintw(win, 2, 0, "╚══════════════════════════════════════════════════════════════════════════════╝");
    mvwprintw(win, 1, 2, "eBPF MAP MONITOR - Live Connection Tracking");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
}

static void draw_stats(WINDOW *win, int map_fd, int y_start) {
    __u64 key, next_key;
    struct conn_info info;
    int count = 0;
    int y = y_start;
    
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, y++, 2, "Active Connections in Map:");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    
    mvwprintw(win, y++, 2, "────────────────────────────────────────────────────────────────────────────");
    
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, y++, 2, "%-6s %-16s %-21s %-21s", 
              "PID", "Process", "Local", "Remote");
    wattroff(win, COLOR_PAIR(3));
    
    key = 0;
    while (bpf_map_get_next_key(map_fd, &key, &next_key) == 0 && count < 15) {
        if (bpf_map_lookup_elem(map_fd, &next_key, &info) == 0) {
            char saddr[32], daddr[32];
            snprintf(saddr, sizeof(saddr), "%u.%u.%u.%u:%u",
                    (info.saddr >> 0) & 0xFF,
                    (info.saddr >> 8) & 0xFF,
                    (info.saddr >> 16) & 0xFF,
                    (info.saddr >> 24) & 0xFF,
                    info.sport);
            snprintf(daddr, sizeof(daddr), "%u.%u.%u.%u:%u",
                    (info.daddr >> 0) & 0xFF,
                    (info.daddr >> 8) & 0xFF,
                    (info.daddr >> 16) & 0xFF,
                    (info.daddr >> 24) & 0xFF,
                    info.dport);
            
            wattron(win, COLOR_PAIR(4));
            mvwprintw(win, y++, 2, "%-6u %-16s %-21s %-21s",
                     info.pid, info.comm, saddr, daddr);
            wattroff(win, COLOR_PAIR(4));
            count++;
        }
        key = next_key;
    }
    
    if (count == 0) {
        wattron(win, COLOR_PAIR(5));
        mvwprintw(win, y++, 2, "  (No active connections)");
        wattroff(win, COLOR_PAIR(5));
    }
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, y + 2, 2, "Total entries: %d", count);
    wattroff(win, COLOR_PAIR(2));
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <map_fd>\n", argv[0]);
        return 1;
    }
    
    int map_fd = atoi(argv[1]);
    
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(500);
    
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    
    WINDOW *win = newwin(LINES, COLS, 0, 0);
    keypad(win, TRUE);
    
    while (1) {
        werase(win);
        draw_header(win);
        draw_stats(win, map_fd, 4);
        
        wattron(win, COLOR_PAIR(5));
        mvwprintw(win, LINES - 2, 2, "Press 'q' to quit | Refresh: 500ms");
        wattroff(win, COLOR_PAIR(5));
        
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == 'q' || ch == 'Q') break;
    }
    
    delwin(win);
    endwin();
    
    return 0;
}
