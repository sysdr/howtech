#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

static volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    running = 0;
}

typedef struct {
    long minflt, majflt;
    long nvcsw, nivcsw;
    unsigned long vsize, rss;
} proc_stats_t;

int get_proc_stats(pid_t pid, proc_stats_t *stats) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    unsigned long utime, stime, cutime, cstime, priority, nice;
    unsigned long num_threads, itrealvalue, starttime;
    unsigned long vsize;
    unsigned long rss_pages;
    unsigned long dummy;  // Some systems have an extra field
    
    int n = fscanf(f, "%*d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                   comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                   &flags, &minflt, &cminflt, &majflt, &cmajflt,
                   &utime, &stime, &cutime, &cstime, &priority, &nice,
                   &num_threads, &itrealvalue, &starttime, &vsize, &dummy, &rss_pages);
    
    fclose(f);
    
    if (n < 23) return -1;
    
    stats->minflt = minflt;
    stats->majflt = majflt;
    stats->vsize = vsize;
    stats->rss = rss_pages * sysconf(_SC_PAGESIZE);
    
    // Get context switches
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (f) {
        char line[256];
        stats->nvcsw = 0;
        stats->nivcsw = 0;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "voluntary_ctxt_switches: %ld", &stats->nvcsw) == 1) continue;
            if (sscanf(line, "nonvoluntary_ctxt_switches: %ld", &stats->nivcsw) == 1) continue;
        }
        fclose(f);
    }
    
    return 0;
}

int count_memory_regions(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    
    int count = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        count++;
    }
    fclose(f);
    return count;
}

void display_memory_map(WINDOW *win, pid_t pid, int start_y) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    
    int y = start_y;
    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    (void)max_x;  // Suppress unused variable warning
    
    char line[512];
    int shown = 0;
    while (fgets(line, sizeof(line), f) && y < max_y - 2 && shown < 15) {
        unsigned long start, end;
        char perms[5];
        
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
            // Show only interesting regions
            if (start < 0x0000800000000000UL) {
                char display[128];
                snprintf(display, sizeof(display), "%.75s", line);
                display[strlen(display)-1] = '\0'; // Remove newline
                
                if (strstr(line, "[stack]")) {
                    wattron(win, COLOR_PAIR(3));
                } else if (strstr(line, "[heap]")) {
                    wattron(win, COLOR_PAIR(2));
                } else if (perms[2] == 'x') {
                    wattron(win, COLOR_PAIR(4));
                } else {
                    wattron(win, COLOR_PAIR(1));
                }
                
                mvwprintw(win, y++, 2, "%s", display);
                wattroff(win, COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
                shown++;
            }
        }
    }
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid = atoi(argv[1]);
    
    signal(SIGINT, sigint_handler);
    
    // Initialize ncurses
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(1000);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    
    proc_stats_t prev_stats = {0}, curr_stats;
    
    while (running) {
        clear();
        
        // Header
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(0, 0, "════════════════════════════════════════════════════════════════");
        mvprintw(1, 0, "   x86-64 Virtual Memory Monitor - PID %d", target_pid);
        mvprintw(2, 0, "════════════════════════════════════════════════════════════════");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        if (get_proc_stats(target_pid, &curr_stats) != 0) {
            mvprintw(4, 2, "Process not found or terminated");
            refresh();
            sleep(1);
            continue;
        }
        
        // Memory statistics
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(4, 2, "MEMORY STATISTICS:");
        attroff(COLOR_PAIR(2) | A_BOLD);
        
        mvprintw(5, 4, "Virtual Size (VSZ): %.2f MB", curr_stats.vsize / 1024.0 / 1024.0);
        mvprintw(6, 4, "Resident Set (RSS): %.2f MB", curr_stats.rss / 1024.0 / 1024.0);
        mvprintw(7, 4, "Memory Regions:     %d", count_memory_regions(target_pid));
        
        // Page faults
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(9, 2, "PAGE FAULTS:");
        attroff(COLOR_PAIR(3) | A_BOLD);
        
        long minflt_delta = curr_stats.minflt - prev_stats.minflt;
        long majflt_delta = curr_stats.majflt - prev_stats.majflt;
        
        mvprintw(10, 4, "Minor (TLB miss):   %ld (Δ%+ld)", curr_stats.minflt, minflt_delta);
        mvprintw(11, 4, "Major (disk I/O):   %ld (Δ%+ld)", curr_stats.majflt, majflt_delta);
        
        // Context switches
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(13, 2, "CONTEXT SWITCHES:");
        attroff(COLOR_PAIR(1) | A_BOLD);
        
        long nvcsw_delta = curr_stats.nvcsw - prev_stats.nvcsw;
        long nivcsw_delta = curr_stats.nivcsw - prev_stats.nivcsw;
        
        mvprintw(14, 4, "Voluntary:          %ld (Δ%+ld)", curr_stats.nvcsw, nvcsw_delta);
        mvprintw(15, 4, "Involuntary:        %ld (Δ%+ld)", curr_stats.nivcsw, nivcsw_delta);
        
        // Memory map preview
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(17, 2, "MEMORY MAP (first 15 regions):");
        attroff(COLOR_PAIR(5) | A_BOLD);
        
        display_memory_map(stdscr, target_pid, 18);
        
        // Footer
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        (void)max_x;  // Suppress unused variable warning
        attron(COLOR_PAIR(4));
        mvprintw(max_y-1, 0, "Press Ctrl+C to exit | Refreshing every second");
        attroff(COLOR_PAIR(4));
        
        prev_stats = curr_stats;
        refresh();
        
        getch(); // Wait for timeout
    }
    
    endwin();
    printf("\nMonitor terminated.\n");
    return 0;
}
