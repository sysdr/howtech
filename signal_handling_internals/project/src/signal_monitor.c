#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t signal_counts[32] = {0};
static volatile sig_atomic_t total_signals = 0;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
    } else if (sig >= 0 && sig < 32) {
        signal_counts[sig]++;
        total_signals++;
    }
}

void setup_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
}

void draw_interface(WINDOW *win, time_t start_time) {
    time_t now = time(NULL);
    int elapsed = (int)(now - start_time);
    
    wclear(win);
    
    // Title
    wattron(win, A_BOLD | COLOR_PAIR(1));
    mvwprintw(win, 0, 2, "╔═══════════════════════════════════════════════════════╗");
    mvwprintw(win, 1, 2, "║   SIGNAL MONITOR - Real-time Signal Statistics        ║");
    mvwprintw(win, 2, 2, "╚═══════════════════════════════════════════════════════╝");
    wattroff(win, A_BOLD | COLOR_PAIR(1));
    
    // Runtime info
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 4, 4, "Runtime: %02d:%02d:%02d", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
    mvwprintw(win, 4, 35, "Total Signals: %d", total_signals);
    wattroff(win, COLOR_PAIR(2));
    
    // Signal statistics
    wattron(win, A_BOLD);
    mvwprintw(win, 6, 4, "Signal");
    mvwprintw(win, 6, 20, "Count");
    mvwprintw(win, 6, 35, "Name");
    wattroff(win, A_BOLD);
    
    int row = 8;
    const char *signal_names[] = {
        [SIGUSR1] = "SIGUSR1",
        [SIGUSR2] = "SIGUSR2",
        [SIGALRM] = "SIGALRM",
        [SIGTERM] = "SIGTERM",
        [SIGINT] = "SIGINT"
    };
    
    for (int sig = 0; sig < 32; sig++) {
        if (signal_counts[sig] > 0) {
            wattron(win, COLOR_PAIR(3));
            mvwprintw(win, row, 4, "%-6d", sig);
            mvwprintw(win, row, 20, "%-8d", signal_counts[sig]);
            mvwprintw(win, row, 35, "%s", 
                     signal_names[sig] ? signal_names[sig] : "UNKNOWN");
            wattroff(win, COLOR_PAIR(3));
            row++;
        }
    }
    
    // Instructions
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, LINES - 3, 4, "Send signals to PID %d to see them tracked", getpid());
    mvwprintw(win, LINES - 2, 4, "Example: kill -USR1 %d", getpid());
    mvwprintw(win, LINES - 1, 4, "Press Ctrl+C to exit");
    wattroff(win, COLOR_PAIR(4));
    
    wrefresh(win);
}

int main(void) {
    setup_handlers();
    
    // Initialize ncurses
    WINDOW *win = initscr();
    cbreak();
    noecho();
    nodelay(win, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
    }
    
    time_t start_time = time(NULL);
    
    while (running) {
        draw_interface(win, start_time);
        usleep(100000); // 100ms refresh
    }
    
    endwin();
    
    printf("\nFinal Statistics:\n");
    printf("Total signals received: %d\n", total_signals);
    for (int sig = 0; sig < 32; sig++) {
        if (signal_counts[sig] > 0) {
            printf("  Signal %d: %d times\n", sig, signal_counts[sig]);
        }
    }
    
    return 0;
}
