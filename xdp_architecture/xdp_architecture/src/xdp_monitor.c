#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <ncurses.h>

#define STAT_RX_PACKETS   0
#define STAT_RX_BYTES     1
#define STAT_DROPPED      2
#define STAT_PASSED       3
#define STAT_REDIRECTED   4

static volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <map_fd>\n", argv[0]);
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int map_fd = atoi(argv[1]);
    
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    
    unsigned long long prev_stats[5] = {0};
    unsigned long long curr_stats[5] = {0};
    
    time_t start_time = time(NULL);
    
    while (running) {
        clear();
        
        /* Read stats from per-CPU map */
        for (int idx = 0; idx < 5; idx++) {
            unsigned long long total = 0;
            unsigned long long values[128] = {0};  /* Max CPUs */
            __u32 key = idx;
            
            if (bpf_map_lookup_elem(map_fd, &key, values) == 0) {
                /* Sum across all CPUs */
                for (int cpu = 0; cpu < 128; cpu++) {
                    total += values[cpu];
                }
            }
            
            curr_stats[idx] = total;
        }
        
        time_t now = time(NULL);
        int uptime = now - start_time;
        
        /* Display header */
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(0, 0, "=== XDP Statistics Monitor ===");
        attroff(COLOR_PAIR(2) | A_BOLD);
        
        mvprintw(1, 0, "Uptime: %d seconds", uptime);
        mvprintw(2, 0, "Press Ctrl+C to exit");
        
        /* Display stats */
        attron(COLOR_PAIR(1));
        mvprintw(4, 0, "Total Packets RX:");
        attroff(COLOR_PAIR(1));
        mvprintw(4, 25, "%20llu", curr_stats[STAT_RX_PACKETS]);
        
        attron(COLOR_PAIR(1));
        mvprintw(5, 0, "Total Bytes RX:");
        attroff(COLOR_PAIR(1));
        mvprintw(5, 25, "%20llu", curr_stats[STAT_RX_BYTES]);
        
        attron(COLOR_PAIR(3));
        mvprintw(7, 0, "Dropped (XDP_DROP):");
        attroff(COLOR_PAIR(3));
        mvprintw(7, 25, "%20llu", curr_stats[STAT_DROPPED]);
        
        attron(COLOR_PAIR(1));
        mvprintw(8, 0, "Passed (XDP_PASS):");
        attroff(COLOR_PAIR(1));
        mvprintw(8, 25, "%20llu", curr_stats[STAT_PASSED]);
        
        /* Calculate rates */
        if (uptime > 0) {
            unsigned long long pps = curr_stats[STAT_RX_PACKETS] / uptime;
            unsigned long long bps = (curr_stats[STAT_RX_BYTES] * 8) / uptime;
            double mbps = bps / 1000000.0;
            unsigned long long drops_per_sec = curr_stats[STAT_DROPPED] / uptime;
            
            attron(COLOR_PAIR(4));
            mvprintw(10, 0, "Average Packet Rate:");
            attroff(COLOR_PAIR(4));
            mvprintw(10, 25, "%15llu pps", pps);
            
            attron(COLOR_PAIR(4));
            mvprintw(11, 0, "Average Bit Rate:");
            attroff(COLOR_PAIR(4));
            mvprintw(11, 25, "%15.2f Mbps", mbps);
            
            attron(COLOR_PAIR(3));
            mvprintw(12, 0, "Drop Rate:");
            attroff(COLOR_PAIR(3));
            mvprintw(12, 25, "%15llu pps", drops_per_sec);
            
            if (curr_stats[STAT_RX_PACKETS] > 0) {
                double drop_percent = (curr_stats[STAT_DROPPED] * 100.0) / curr_stats[STAT_RX_PACKETS];
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(13, 0, "Drop Percentage:");
                attroff(COLOR_PAIR(3) | A_BOLD);
                mvprintw(13, 25, "%14.2f %%", drop_percent);
            }
        }
        
        /* Display XDP mode hint */
        attron(COLOR_PAIR(2));
        mvprintw(15, 0, "Check XDP mode with: ip link show dev lo");
        attroff(COLOR_PAIR(2));
        mvprintw(16, 0, "  xdpgeneric = Generic (slow)");
        mvprintw(17, 0, "  xdpdrv     = Native (fast)");
        mvprintw(18, 0, "  xdpoffload = Offloaded (SmartNIC)");
        
        refresh();
        usleep(500000);  /* Update every 500ms */
    }
    
    endwin();
    
    printf("\nFinal Statistics:\n");
    printf("  RX Packets: %llu\n", curr_stats[STAT_RX_PACKETS]);
    printf("  RX Bytes:   %llu\n", curr_stats[STAT_RX_BYTES]);
    printf("  Dropped:    %llu\n", curr_stats[STAT_DROPPED]);
    printf("  Passed:     %llu\n", curr_stats[STAT_PASSED]);
    
    return 0;
}
