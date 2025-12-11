#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <time.h>

#define UPDATE_INTERVAL_MS 200

typedef struct {
    long nr_dirty;
    long nr_writeback;
    long nr_written;
    long pgpgout;
} vmstat_t;

void die(const char *msg) {
    endwin();
    perror(msg);
    exit(1);
}

int read_vmstat(vmstat_t *stats) {
    FILE *fp = fopen("/proc/vmstat", "r");
    if (!fp) return -1;
    
    char line[256];
    memset(stats, 0, sizeof(*stats));
    
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "nr_dirty %ld", &stats->nr_dirty) == 1) continue;
        if (sscanf(line, "nr_writeback %ld", &stats->nr_writeback) == 1) continue;
        if (sscanf(line, "nr_written %ld", &stats->nr_written) == 1) continue;
        if (sscanf(line, "pgpgout %ld", &stats->pgpgout) == 1) continue;
    }
    
    fclose(fp);
    return 0;
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    }
    
    vmstat_t stats, prev_stats;
    memset(&prev_stats, 0, sizeof(prev_stats));
    
    time_t start_time = time(NULL);
    
    while (1) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        
        if (read_vmstat(&stats) < 0) die("read_vmstat");
        
        clear();
        
        // Header
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(0, 0, "╔═══════════════════════════════════════════════════════════════════╗");
        mvprintw(1, 0, "║         DIRTY PAGE WRITEBACK MONITOR (press 'q' to quit)         ║");
        mvprintw(2, 0, "╚═══════════════════════════════════════════════════════════════════╝");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        time_t uptime = time(NULL) - start_time;
        mvprintw(3, 2, "Runtime: %ld seconds", uptime);
        
        // Current stats
        mvprintw(5, 2, "CURRENT PAGE STATE:");
        attron(A_BOLD);
        mvprintw(6, 2, "─────────────────────────────────────────────────────────────────");
        attroff(A_BOLD);
        
        // Dirty pages (color coded)
        mvprintw(7, 4, "Dirty pages:    ");
        if (stats.nr_dirty > 10000) {
            attron(COLOR_PAIR(3) | A_BOLD);
            printw("%8ld", stats.nr_dirty);
            attroff(COLOR_PAIR(3) | A_BOLD);
            printw("  [HIGH]");
        } else if (stats.nr_dirty > 1000) {
            attron(COLOR_PAIR(2) | A_BOLD);
            printw("%8ld", stats.nr_dirty);
            attroff(COLOR_PAIR(2) | A_BOLD);
            printw("  [MEDIUM]");
        } else {
            attron(COLOR_PAIR(1));
            printw("%8ld", stats.nr_dirty);
            attroff(COLOR_PAIR(1));
            printw("  [LOW]");
        }
        printw("  (%.2f MB)", stats.nr_dirty * 4.0 / 1024.0);
        
        // Writeback in progress
        mvprintw(8, 4, "Writeback:      ");
        if (stats.nr_writeback > 0) {
            attron(COLOR_PAIR(5) | A_BOLD);
            printw("%8ld", stats.nr_writeback);
            attroff(COLOR_PAIR(5) | A_BOLD);
            printw("  [ACTIVE]");
        } else {
            printw("%8ld", stats.nr_writeback);
            printw("  [IDLE]");
        }
        printw("  (%.2f MB)", stats.nr_writeback * 4.0 / 1024.0);
        
        // Total written
        mvprintw(9, 4, "Total written:  %8ld pages (%.2f MB)",
                 stats.nr_written, stats.nr_written * 4.0 / 1024.0);
        
        // Pages written to disk
        mvprintw(10, 4, "Disk writes:    %8ld operations", stats.pgpgout);
        
        // Delta stats
        mvprintw(12, 2, "CHANGES SINCE LAST UPDATE:");
        attron(A_BOLD);
        mvprintw(13, 2, "─────────────────────────────────────────────────────────────────");
        attroff(A_BOLD);
        
        long delta_dirty = stats.nr_dirty - prev_stats.nr_dirty;
        long delta_written = stats.nr_written - prev_stats.nr_written;
        long delta_pgpgout = stats.pgpgout - prev_stats.pgpgout;
        
        mvprintw(14, 4, "Dirty delta:    ");
        if (delta_dirty > 0) {
            attron(COLOR_PAIR(2));
            printw("+%ld", delta_dirty);
            attroff(COLOR_PAIR(2));
        } else if (delta_dirty < 0) {
            attron(COLOR_PAIR(1));
            printw("%ld", delta_dirty);
            attroff(COLOR_PAIR(1));
        } else {
            printw("0");
        }
        
        mvprintw(15, 4, "Written delta:  %+ld pages", delta_written);
        mvprintw(16, 4, "Disk I/O delta: %+ld operations", delta_pgpgout);
        
        // Visual bar for dirty pages
        mvprintw(18, 2, "DIRTY PAGE LEVEL:");
        int bar_width = 50;
        int filled = (stats.nr_dirty * bar_width) / 20000;  // Assuming 20k as max for visualization
        if (filled > bar_width) filled = bar_width;
        
        mvprintw(19, 4, "[");
        for (int i = 0; i < filled; i++) {
            if (i < bar_width * 0.5) attron(COLOR_PAIR(1));
            else if (i < bar_width * 0.8) attron(COLOR_PAIR(2));
            else attron(COLOR_PAIR(3));
            printw("█");
            if (i < bar_width * 0.5) attroff(COLOR_PAIR(1));
            else if (i < bar_width * 0.8) attroff(COLOR_PAIR(2));
            else attroff(COLOR_PAIR(3));
        }
        for (int i = filled; i < bar_width; i++) {
            printw("─");
        }
        printw("]");
        
        // Instructions
        mvprintw(22, 2, "To generate dirty pages, run:");
        attron(COLOR_PAIR(4));
        mvprintw(23, 4, "./dirty_generator");
        attroff(COLOR_PAIR(4));
        
        refresh();
        
        prev_stats = stats;
        usleep(UPDATE_INTERVAL_MS * 1000);
    }
    
    endwin();
    return 0;
}
