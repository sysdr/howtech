#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>

#define UPDATE_INTERVAL_MS 500

void draw_header(WINDOW *win)
{
    wattron(win, A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, 0, 2, "eBPF JIT COMPILATION MONITOR");
    wattroff(win, A_BOLD | COLOR_PAIR(1));
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 1, 2, "Real-time Performance Analysis");
    wattroff(win, COLOR_PAIR(2));
}

void draw_box(WINDOW *win, int y, int x, int height, int width, const char *title, int color_pair)
{
    wattron(win, COLOR_PAIR(color_pair));
    box(win, 0, 0);
    if (title) {
        mvwprintw(win, 0, 3, " %s ", title);
    }
    wattroff(win, COLOR_PAIR(color_pair));
}

int read_jit_status(void)
{
    FILE *fp = fopen("/proc/sys/net/core/bpf_jit_enable", "r");
    if (!fp) return -1;
    
    int enabled;
    if (fscanf(fp, "%d", &enabled) != 1) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return enabled;
}

void simulate_metrics(double *interpreter_pps, double *jit_pps, double *cpu_usage)
{
    static int iteration = 0;
    iteration++;
    
    // Simulated metrics with realistic values
    *interpreter_pps = 2.3 + (rand() % 100) / 1000.0;
    *jit_pps = 11.5 + (rand() % 150) / 1000.0;
    *cpu_usage = 45.0 + (rand() % 200) / 10.0;
}

int main(void)
{
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(UPDATE_INTERVAL_MS);
    
    // Color pairs
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Create windows
    WINDOW *header_win = newwin(3, max_x - 2, 1, 1);
    WINDOW *status_win = newwin(8, max_x - 2, 4, 1);
    WINDOW *perf_win = newwin(12, max_x - 2, 12, 1);
    WINDOW *info_win = newwin(8, max_x - 2, 24, 1);
    
    double interpreter_pps = 0, jit_pps = 0, cpu_usage = 0;
    time_t start_time = time(NULL);
    
    while (1) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        
        // Clear windows
        werase(header_win);
        werase(status_win);
        werase(perf_win);
        werase(info_win);
        
        // Draw header
        draw_header(header_win);
        
        // JIT Status Box
        draw_box(status_win, 0, 0, 8, max_x - 2, "JIT STATUS", 3);
        int jit_enabled = read_jit_status();
        
        if (jit_enabled == 1) {
            wattron(status_win, A_BOLD | COLOR_PAIR(3));
            mvwprintw(status_win, 2, 3, "JIT Compilation: ENABLED");
            wattroff(status_win, A_BOLD | COLOR_PAIR(3));
        } else if (jit_enabled == 0) {
            wattron(status_win, A_BOLD | COLOR_PAIR(5));
            mvwprintw(status_win, 2, 3, "JIT Compilation: DISABLED");
            wattroff(status_win, A_BOLD | COLOR_PAIR(5));
        } else {
            wattron(status_win, COLOR_PAIR(4));
            mvwprintw(status_win, 2, 3, "JIT Compilation: UNKNOWN");
            wattroff(status_win, COLOR_PAIR(4));
        }
        
        mvwprintw(status_win, 3, 3, "Mode: %s", jit_enabled ? "Native x86-64 Execution" : "Bytecode Interpreter");
        mvwprintw(status_win, 4, 3, "Uptime: %ld seconds", time(NULL) - start_time);
        
        wattron(status_win, COLOR_PAIR(2));
        mvwprintw(status_win, 6, 3, "Toggle: echo 0/1 > /proc/sys/net/core/bpf_jit_enable");
        wattroff(status_win, COLOR_PAIR(2));
        
        // Performance Metrics Box
        draw_box(perf_win, 0, 0, 12, max_x - 2, "PERFORMANCE COMPARISON", 6);
        
        simulate_metrics(&interpreter_pps, &jit_pps, &cpu_usage);
        
        wattron(perf_win, A_BOLD);
        mvwprintw(perf_win, 2, 3, "Metric");
        mvwprintw(perf_win, 2, 35, "Interpreter");
        mvwprintw(perf_win, 2, 55, "JIT");
        mvwprintw(perf_win, 2, 70, "Speedup");
        wattroff(perf_win, A_BOLD);
        
        mvwhline(perf_win, 3, 3, ACS_HLINE, max_x - 8);
        
        wattron(perf_win, COLOR_PAIR(5));
        mvwprintw(perf_win, 4, 3, "Throughput (M pps)");
        mvwprintw(perf_win, 4, 35, "%.2f", interpreter_pps);
        wattroff(perf_win, COLOR_PAIR(5));
        
        wattron(perf_win, COLOR_PAIR(3));
        mvwprintw(perf_win, 4, 55, "%.2f", jit_pps);
        wattroff(perf_win, COLOR_PAIR(3));
        
        wattron(perf_win, A_BOLD | COLOR_PAIR(3));
        mvwprintw(perf_win, 4, 70, "%.1fx", jit_pps / interpreter_pps);
        wattroff(perf_win, A_BOLD | COLOR_PAIR(3));
        
        mvwprintw(perf_win, 5, 3, "Dispatch Overhead");
        wattron(perf_win, COLOR_PAIR(5));
        mvwprintw(perf_win, 5, 35, "~250ns");
        wattroff(perf_win, COLOR_PAIR(5));
        wattron(perf_win, COLOR_PAIR(3));
        mvwprintw(perf_win, 5, 55, "~5ns");
        wattroff(perf_win, COLOR_PAIR(3));
        
        mvwprintw(perf_win, 6, 3, "CPU Usage (%)");
        mvwprintw(perf_win, 6, 35, "%.1f%%", cpu_usage * 1.8);
        mvwprintw(perf_win, 6, 55, "%.1f%%", cpu_usage);
        
        mvwprintw(perf_win, 8, 3, "I-Cache Behavior:");
        wattron(perf_win, COLOR_PAIR(4));
        mvwprintw(perf_win, 9, 5, "• Interpreter: Bytecode + dispatch compete for cache lines");
        wattroff(perf_win, COLOR_PAIR(4));
        wattron(perf_win, COLOR_PAIR(3));
        mvwprintw(perf_win, 10, 5, "• JIT: Hot native code, predictable branches");
        wattroff(perf_win, COLOR_PAIR(3));
        
        // Information Box
        draw_box(info_win, 0, 0, 8, max_x - 2, "COMMANDS & INFO", 2);
        mvwprintw(info_win, 2, 3, "Inspect JIT code:  bpftool prog dump jited id <ID>");
        mvwprintw(info_win, 3, 3, "View symbols:      cat /proc/kallsyms | grep bpf_prog");
        mvwprintw(info_win, 4, 3, "Profile:           perf record -g -- <command>");
        mvwprintw(info_win, 6, 3, "Press 'q' to quit");
        
        // Refresh all windows
        wrefresh(header_win);
        wrefresh(status_win);
        wrefresh(perf_win);
        wrefresh(info_win);
    }
    
    // Cleanup
    delwin(header_win);
    delwin(status_win);
    delwin(perf_win);
    delwin(info_win);
    endwin();
    
    return 0;
}
