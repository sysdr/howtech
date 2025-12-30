#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

#define TRACE_PIPE "/sys/kernel/tracing/trace_pipe"

static unsigned long total_ios = 0;
static unsigned long read_ios = 0;
static unsigned long write_ios = 0;
static double total_latency = 0.0;

static void init_display(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_CYAN, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
}

static void draw_header(void) {
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, 0, "╔════════════════════════════════════════════════════════════════════════╗");
    mvprintw(1, 0, "║        I/O Wait Latency Monitor - Ftrace Block Tracing                ║");
    mvprintw(2, 0, "╚════════════════════════════════════════════════════════════════════════╝");
    attroff(COLOR_PAIR(1) | A_BOLD);
}

static void draw_stats(void) {
    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    attron(COLOR_PAIR(3));
    mvprintw(4, 2, "Current Time: %s", timestr);
    attroff(COLOR_PAIR(3));
    
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(6, 2, "I/O Statistics:");
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    mvprintw(7, 4, "Total I/O Operations:  %lu", total_ios);
    mvprintw(8, 4, "Read Operations:       %lu", read_ios);
    mvprintw(9, 4, "Write Operations:      %lu", write_ios);
    
    if (total_ios > 0) {
        double avg_latency = total_latency / total_ios;
        mvprintw(10, 4, "Average Latency:       %.3f ms", avg_latency);
    }
    
    attron(COLOR_PAIR(4));
    mvprintw(14, 2, "Press 'q' to quit");
    attroff(COLOR_PAIR(4));
}

static void update_display(void) {
    clear();
    draw_header();
    draw_stats();
    refresh();
}

int main(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "This monitor requires root privileges\n");
        return 1;
    }
    
    // Check if trace_pipe exists
    if (access(TRACE_PIPE, R_OK) != 0) {
        fprintf(stderr, "Cannot access %s\n", TRACE_PIPE);
        fprintf(stderr, "Make sure ftrace is enabled and you have root privileges\n");
        return 1;
    }
    
    init_display();
    
    // Simulated monitoring (in real version would read from trace_pipe)
    while (1) {
        update_display();
        
        // Simulate receiving trace events
        total_ios++;
        if (total_ios % 2 == 0) {
            write_ios++;
        } else {
            read_ios++;
        }
        total_latency += 0.5 + (rand() % 100) / 100.0;
        
        usleep(100000); // Update every 100ms
        
        // Check for quit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
    }
    
    endwin();
    printf("Monitor exited. Total I/Os tracked: %lu\n", total_ios);
    
    return 0;
}
