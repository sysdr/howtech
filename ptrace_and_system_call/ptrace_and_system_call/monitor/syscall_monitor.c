/*
 * Real-time syscall monitor using ncurses
 * Parses strace output and displays statistics
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <ctype.h>

#define MAX_SYSCALLS 64
#define MAX_LINE 1024

typedef struct {
    char name[64];
    int count;
    int failed;
    double total_time;
} SyscallStats;

SyscallStats stats[MAX_SYSCALLS];
int num_syscalls = 0;
int total_calls = 0;
int total_failures = 0;

void init_monitor(void) {
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
}

int find_or_create_syscall(const char *name) {
    for (int i = 0; i < num_syscalls; i++) {
        if (strcmp(stats[i].name, name) == 0) {
            return i;
        }
    }
    
    if (num_syscalls < MAX_SYSCALLS) {
        strncpy(stats[num_syscalls].name, name, sizeof(stats[0].name) - 1);
        stats[num_syscalls].count = 0;
        stats[num_syscalls].failed = 0;
        stats[num_syscalls].total_time = 0.0;
        return num_syscalls++;
    }
    
    return -1;
}

void parse_strace_line(const char *line) {
    char syscall[64] = {0};
    double elapsed = 0.0;
    int is_failure = 0;
    
    // Extract syscall name
    const char *paren = strchr(line, '(');
    if (paren) {
        size_t len = paren - line;
        if (len > 0 && len < sizeof(syscall)) {
            // Skip leading whitespace and PID
            const char *start = line;
            while (*start && (isspace(*start) || isdigit(*start))) start++;
            len = paren - start;
            if (len < sizeof(syscall)) {
                strncpy(syscall, start, len);
                syscall[len] = '\0';
            }
        }
    }
    
    // Check for failure (= -1 or specific errno)
    if (strstr(line, "= -1") || strstr(line, "ENOENT") || 
        strstr(line, "EACCES") || strstr(line, "EPERM") ||
        strstr(line, "ECONNREFUSED") || strstr(line, "EMFILE")) {
        is_failure = 1;
    }
    
    // Extract timing if present <0.000123>
    const char *time_start = strchr(line, '<');
    if (time_start && strchr(time_start, '>')) {
        sscanf(time_start, "<%lf>", &elapsed);
    }
    
    if (syscall[0]) {
        int idx = find_or_create_syscall(syscall);
        if (idx >= 0) {
            stats[idx].count++;
            stats[idx].total_time += elapsed;
            if (is_failure) {
                stats[idx].failed++;
                total_failures++;
            }
            total_calls++;
        }
    }
}

void display_stats(void) {
    clear();
    
    // Header
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(0, 0, "                    SYSCALL MONITOR - Real-time strace Analysis                    ");
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    attron(COLOR_PAIR(4));
    mvprintw(1, 0, "Time: %s", time_str);
    attroff(COLOR_PAIR(4));
    
    attron(COLOR_PAIR(1));
    mvprintw(1, 30, "Total Calls: %d", total_calls);
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(1, 50, "Failures: %d", total_failures);
    attroff(COLOR_PAIR(2));
    
    double failure_rate = total_calls > 0 ? (100.0 * total_failures / total_calls) : 0.0;
    attron(COLOR_PAIR(3));
    mvprintw(1, 67, "Failure Rate: %.1f%%", failure_rate);
    attroff(COLOR_PAIR(3));
    
    // Column headers
    attron(A_BOLD);
    mvprintw(3, 0, "SYSCALL");
    mvprintw(3, 20, "CALLS");
    mvprintw(3, 30, "FAILED");
    mvprintw(3, 40, "FAIL%%");
    mvprintw(3, 50, "AVG TIME (ms)");
    mvprintw(3, 68, "TOTAL TIME (ms)");
    attroff(A_BOLD);
    
    mvhline(4, 0, ACS_HLINE, 80);
    
    // Sort by count (descending)
    for (int i = 0; i < num_syscalls - 1; i++) {
        for (int j = i + 1; j < num_syscalls; j++) {
            if (stats[j].count > stats[i].count) {
                SyscallStats temp = stats[i];
                stats[i] = stats[j];
                stats[j] = temp;
            }
        }
    }
    
    // Display stats
    int row = 5;
    for (int i = 0; i < num_syscalls && row < LINES - 3; i++) {
        double fail_pct = stats[i].count > 0 ? 
                         (100.0 * stats[i].failed / stats[i].count) : 0.0;
        double avg_time = stats[i].count > 0 ? 
                         (stats[i].total_time * 1000.0 / stats[i].count) : 0.0;
        double total_time_ms = stats[i].total_time * 1000.0;
        
        if (stats[i].failed > 0) {
            attron(COLOR_PAIR(2));
        }
        
        mvprintw(row, 0, "%-18s", stats[i].name);
        mvprintw(row, 20, "%6d", stats[i].count);
        mvprintw(row, 30, "%6d", stats[i].failed);
        mvprintw(row, 40, "%5.1f%%", fail_pct);
        mvprintw(row, 50, "%10.3f", avg_time);
        mvprintw(row, 68, "%12.3f", total_time_ms);
        
        if (stats[i].failed > 0) {
            attroff(COLOR_PAIR(2));
        }
        
        row++;
    }
    
    // Footer
    mvhline(LINES - 2, 0, ACS_HLINE, 80);
    attron(COLOR_PAIR(4));
    mvprintw(LINES - 1, 0, "Press 'q' to quit | Updates every 500ms | Monitoring strace output...");
    attroff(COLOR_PAIR(4));
    
    refresh();
}

int main(void) {
    init_monitor();
    
    char line[MAX_LINE];
    int quit = 0;
    
    while (!quit) {
        // Check for keyboard input
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            quit = 1;
            break;
        }
        
        // Read stdin with timeout
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms
        
        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(line, sizeof(line), stdin)) {
                parse_strace_line(line);
            } else {
                // EOF reached
                break;
            }
        }
        
        display_stats();
        usleep(500000);  // Update display every 500ms
    }
    
    endwin();
    
    // Print summary to stdout
    printf("\n=== Syscall Summary ===\n");
    printf("Total calls: %d\n", total_calls);
    printf("Total failures: %d (%.1f%%)\n", total_failures, 
           total_calls > 0 ? (100.0 * total_failures / total_calls) : 0.0);
    printf("\nTop failing syscalls:\n");
    for (int i = 0; i < num_syscalls && i < 10; i++) {
        if (stats[i].failed > 0) {
            printf("  %s: %d/%d failed (%.1f%%)\n",
                   stats[i].name, stats[i].failed, stats[i].count,
                   100.0 * stats[i].failed / stats[i].count);
        }
    }
    
    return 0;
}
