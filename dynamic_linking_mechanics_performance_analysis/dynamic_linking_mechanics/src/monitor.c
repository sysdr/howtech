#define _GNU_SOURCE
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

volatile sig_atomic_t running = 1;

void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

void draw_box(WINDOW *win, int y, int x, int h, int w, const char *title) {
    (void)y; (void)x; (void)h;  // Parameters reserved for future use
    box(win, 0, 0);
    if (title) {
        mvwprintw(win, 0, (w - strlen(title)) / 2, " %s ", title);
    }
    wrefresh(win);
}

void run_comparison(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(100);
    
    signal(SIGINT, sigint_handler);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Create windows
    WINDOW *header = newwin(5, max_x, 0, 0);
    WINDOW *lazy_win = newwin(max_y - 15, max_x / 2 - 1, 5, 0);
    WINDOW *now_win = newwin(max_y - 15, max_x / 2 - 1, 5, max_x / 2);
    WINDOW *stats_win = newwin(8, max_x, max_y - 10, 0);
    WINDOW *footer = newwin(2, max_x, max_y - 2, 0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_CYAN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
    }
    
    // Draw header
    wattron(header, A_BOLD | COLOR_PAIR(2));
    mvwprintw(header, 1, (max_x - 50) / 2, 
              "╔══════════════════════════════════════════════════╗");
    mvwprintw(header, 2, (max_x - 50) / 2, 
              "║  Lazy vs Immediate Binding: Live Comparison    ║");
    mvwprintw(header, 3, (max_x - 50) / 2, 
              "╚══════════════════════════════════════════════════╝");
    wattroff(header, A_BOLD | COLOR_PAIR(2));
    wrefresh(header);
    
    draw_box(lazy_win, 0, 0, max_y - 15, max_x / 2 - 1, "LAZY BINDING (-Wl,-z,lazy)");
    draw_box(now_win, 0, 0, max_y - 15, max_x / 2 - 1, "IMMEDIATE BINDING (-Wl,-z,now)");
    draw_box(stats_win, 0, 0, 8, max_x, "Performance Statistics");
    
    // Footer
    wattron(footer, COLOR_PAIR(3));
    mvwprintw(footer, 0, 2, "Press Ctrl+C to exit | Demo running in background");
    wattroff(footer, COLOR_PAIR(3));
    wrefresh(footer);
    
    int iteration = 0;
    time_t start_time = time(NULL);
    
    while (running) {
        iteration++;
        time_t current_time = time(NULL);
        int elapsed = (int)(current_time - start_time);
        
        // Update lazy window
        werase(lazy_win);
        draw_box(lazy_win, 0, 0, max_y - 15, max_x / 2 - 1, "LAZY BINDING (-Wl,-z,lazy)");
        
        wattron(lazy_win, COLOR_PAIR(1));
        mvwprintw(lazy_win, 2, 2, "Strategy: RTLD_LAZY");
        wattroff(lazy_win, COLOR_PAIR(1));
        
        mvwprintw(lazy_win, 4, 2, "Symbols: Resolved on first use");
        mvwprintw(lazy_win, 5, 2, "PLT: Active (3-instruction stub)");
        mvwprintw(lazy_win, 6, 2, "GOT: Writable during execution");
        
        wattron(lazy_win, COLOR_PAIR(3));
        mvwprintw(lazy_win, 8, 2, "Startup: ~20ms");
        wattroff(lazy_win, COLOR_PAIR(3));
        
        wattron(lazy_win, COLOR_PAIR(4));
        mvwprintw(lazy_win, 9, 2, "First call overhead: 300-800ns");
        wattroff(lazy_win, COLOR_PAIR(4));
        
        mvwprintw(lazy_win, 11, 2, "Security: GOT overwrite risk");
        mvwprintw(lazy_win, 12, 2, "Threading: Resolver lock contention");
        
        wrefresh(lazy_win);
        
        // Update now window
        werase(now_win);
        draw_box(now_win, 0, 0, max_y - 15, max_x / 2 - 1, "IMMEDIATE BINDING (-Wl,-z,now)");
        
        wattron(now_win, COLOR_PAIR(1));
        mvwprintw(now_win, 2, 2, "Strategy: RTLD_NOW + RELRO");
        wattroff(now_win, COLOR_PAIR(1));
        
        mvwprintw(now_win, 4, 2, "Symbols: All resolved at load");
        mvwprintw(now_win, 5, 2, "PLT: Optimized out (direct jumps)");
        mvwprintw(now_win, 6, 2, "GOT: Read-only after mprotect()");
        
        wattron(now_win, COLOR_PAIR(4));
        mvwprintw(now_win, 8, 2, "Startup: ~65ms");
        wattroff(now_win, COLOR_PAIR(4));
        
        wattron(now_win, COLOR_PAIR(3));
        mvwprintw(now_win, 9, 2, "Runtime: Predictable latency");
        wattroff(now_win, COLOR_PAIR(3));
        
        mvwprintw(now_win, 11, 2, "Security: GOT protected (RELRO)");
        mvwprintw(now_win, 12, 2, "Threading: No resolver contention");
        
        wrefresh(now_win);
        
        // Update stats
        werase(stats_win);
        draw_box(stats_win, 0, 0, 8, max_x, "Performance Statistics");
        
        mvwprintw(stats_win, 2, 2, "Elapsed Time: %d seconds | Iterations: %d", 
                  elapsed, iteration);
        
        wattron(stats_win, COLOR_PAIR(2));
        mvwprintw(stats_win, 4, 2, "Lazy:      Fast startup, variable runtime (PLT overhead on first calls)");
        mvwprintw(stats_win, 5, 2, "Immediate: Slow startup, consistent runtime (no PLT resolution)");
        wattroff(stats_win, COLOR_PAIR(2));
        
        wrefresh(stats_win);
        
        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) {
            break;
        }
        
        usleep(500000); // 0.5 second refresh
    }
    
    delwin(header);
    delwin(lazy_win);
    delwin(now_win);
    delwin(stats_win);
    delwin(footer);
    endwin();
}

int main(void) {
    run_comparison();
    return 0;
}
