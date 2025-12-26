/*
 * klog_monitor.c - Real-time kernel log monitor
 * Displays kernel messages with color coding by log level
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/klog.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>
#include <errno.h>

#define KLOG_BUF_SIZE 16384
#define MAX_LINES 100

/* Log level colors */
#define COLOR_EMERG   1  /* Red */
#define COLOR_ALERT   2  /* Bright Red */
#define COLOR_CRIT    3  /* Magenta */
#define COLOR_ERR     4  /* Yellow */
#define COLOR_WARN    5  /* Cyan */
#define COLOR_NOTICE  6  /* Green */
#define COLOR_INFO    7  /* White */
#define COLOR_DEBUG   8  /* Gray */

static volatile sig_atomic_t running = 1;

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        running = 0;
    }
}

void init_colors(void) {
    start_color();
    init_pair(COLOR_EMERG,  COLOR_RED,     COLOR_BLACK);
    init_pair(COLOR_ALERT,  COLOR_RED,     COLOR_BLACK);
    init_pair(COLOR_CRIT,   COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_ERR,    COLOR_YELLOW,  COLOR_BLACK);
    init_pair(COLOR_WARN,   COLOR_CYAN,    COLOR_BLACK);
    init_pair(COLOR_NOTICE, COLOR_GREEN,   COLOR_BLACK);
    init_pair(COLOR_INFO,   COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_DEBUG,  COLOR_WHITE,   COLOR_BLACK);
}

int get_log_level_from_line(const char *line, int *color_pair) {
    /* Parse kernel log level from message */
    if (strstr(line, "EMERG") || strstr(line, "<0>")) {
        *color_pair = COLOR_EMERG;
        return 0;
    } else if (strstr(line, "ALERT") || strstr(line, "<1>")) {
        *color_pair = COLOR_ALERT;
        return 1;
    } else if (strstr(line, "CRIT") || strstr(line, "<2>")) {
        *color_pair = COLOR_CRIT;
        return 2;
    } else if (strstr(line, "ERR") || strstr(line, "<3>")) {
        *color_pair = COLOR_ERR;
        return 3;
    } else if (strstr(line, "WARNING") || strstr(line, "<4>")) {
        *color_pair = COLOR_WARN;
        return 4;
    } else if (strstr(line, "NOTICE") || strstr(line, "<5>")) {
        *color_pair = COLOR_NOTICE;
        return 5;
    } else if (strstr(line, "INFO") || strstr(line, "<6>")) {
        *color_pair = COLOR_INFO;
        return 6;
    } else if (strstr(line, "DEBUG") || strstr(line, "<7>")) {
        *color_pair = COLOR_DEBUG;
        return 7;
    }
    
    /* Default to INFO */
    *color_pair = COLOR_INFO;
    return 6;
}

void display_header(WINDOW *win, int height, int width) {
    wattron(win, A_BOLD | COLOR_PAIR(COLOR_NOTICE));
    mvwprintw(win, 0, 0, "╔");
    for (int i = 1; i < width - 1; i++) waddch(win, '═');
    waddch(win, '╗');
    
    mvwprintw(win, 1, 2, "Kernel Log Monitor - Real-time printk() Output");
    mvwprintw(win, 2, 0, "╠");
    for (int i = 1; i < width - 1; i++) waddch(win, '═');
    waddch(win, '╣');
    
    wattroff(win, A_BOLD | COLOR_PAIR(COLOR_NOTICE));
    
    /* Stats line */
    time_t now = time(NULL);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    mvwprintw(win, 1, width - 22, "%s", timebuf);
    
    wrefresh(win);
}

void display_footer(WINDOW *win, int height, int width, int msg_count) {
    wattron(win, COLOR_PAIR(COLOR_INFO));
    mvwprintw(win, height - 2, 0, "╚");
    for (int i = 1; i < width - 1; i++) waddch(win, '═');
    waddch(win, '╝');
    
    mvwprintw(win, height - 1, 2, "Messages: %d | Press Ctrl+C to exit | Monitoring /dev/kmsg", msg_count);
    wattroff(win, COLOR_PAIR(COLOR_INFO));
    
    wrefresh(win);
}

int main(void) {
    int fd;
    char buffer[KLOG_BUF_SIZE];
    int msg_count = 0;
    
    /* Setup signal handling */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
    
    int height, width;
    getmaxyx(stdscr, height, width);
    
    init_colors();
    
    /* Open kernel log (requires CAP_SYSLOG or root) */
    fd = open("/dev/kmsg", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        endwin();
        fprintf(stderr, "Failed to open /dev/kmsg: %s\n", strerror(errno));
        fprintf(stderr, "Try running with sudo\n");
        return 1;
    }
    
    int line_offset = 3;  /* Start after header */
    int current_line = line_offset;
    
    display_header(stdscr, height, width);
    display_footer(stdscr, height, width, msg_count);
    
    /* Main monitoring loop */
    while (running) {
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        
        if (n > 0) {
            buffer[n] = '\0';
            
            /* Split into lines */
            char *line = strtok(buffer, "\n");
            while (line != NULL && running) {
                int color_pair;
                get_log_level_from_line(line, &color_pair);
                
                /* Display the line */
                if (current_line >= height - 3) {
                    /* Scroll up */
                    current_line = line_offset;
                    for (int i = line_offset; i < height - 3; i++) {
                        wmove(stdscr, i, 0);
                        wclrtoeol(stdscr);
                    }
                }
                
                wattron(stdscr, COLOR_PAIR(color_pair));
                mvwprintw(stdscr, current_line, 1, "%-*.*s", 
                         width - 2, width - 2, line);
                wattroff(stdscr, COLOR_PAIR(color_pair));
                
                current_line++;
                msg_count++;
                
                line = strtok(NULL, "\n");
            }
            
            display_footer(stdscr, height, width, msg_count);
            refresh();
        }
        
        /* Check for user input */
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = 0;
        }
        
        /* Small delay to prevent CPU spinning */
        usleep(50000);  /* 50ms */
    }
    
    close(fd);
    endwin();
    
    printf("Monitor stopped. Received %d messages.\n", msg_count);
    
    return 0;
}
