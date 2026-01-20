#define _GNU_SOURCE
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <time.h>

#define NUM_SYSCALLS 450

// Shared memory segment for syscall statistics
// This would typically use shared memory, but for simplicity
// we'll read from a file or use IPC. For now, this is a placeholder.
// In a real implementation, this would connect to shared memory
// or a message queue from syscall_test.

const char* syscall_name(long nr) {
    switch(nr) {
        case SYS_read: return "read";
        case SYS_write: return "write";
        case SYS_open: return "open";
        case SYS_close: return "close";
        case SYS_getpid: return "getpid";
        case SYS_gettid: return "gettid";
        case SYS_clock_gettime: return "clock_gettime";
        case SYS_openat: return "openat";
        case SYS_unlinkat: return "unlinkat";
        default: return "unknown";
    }
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    
    curs_set(0);
    
    int ch;
    int update_count = 0;
    time_t start_time = time(NULL);
    
    while ((ch = getch()) != 'q' && ch != 'Q') {
        if (ch == ERR) {
            // No key pressed, update display
            clear();
            
            mvprintw(0, 0, "System Call Monitor (Press 'q' to quit)");
            mvprintw(1, 0, "========================================");
            mvprintw(2, 0, "Update #%d | Uptime: %ld seconds", update_count++, (long)(time(NULL) - start_time));
            mvprintw(3, 0, "");
            
            mvprintw(4, 0, "%-20s %10s %12s %12s", "Syscall", "Count", "Total Cycles", "Avg Cycles");
            mvprintw(5, 0, "%-20s %10s %12s %12s", "--------------------", "----------", "------------", "------------");
            
            int line = 6;
            // Display placeholder data
            // In real implementation, this would read from shared memory
            mvprintw(line++, 0, "%-20s %10s %12s %12s", 
                     "getpid", "0", "0", "0");
            mvprintw(line++, 0, "%-20s %10s %12s %12s", 
                     "gettid", "0", "0", "0");
            mvprintw(line++, 0, "%-20s %10s %12s %12s", 
                     "write", "0", "0", "0");
            mvprintw(line++, 0, "%-20s %10s %12s %12s", 
                     "close", "0", "0", "0");
            mvprintw(line++, 0, "%-20s %10s %12s %12s", 
                     "openat", "0", "0", "0");
            
            mvprintw(LINES - 2, 0, "");
            mvprintw(LINES - 1, 0, "Note: This is a placeholder monitor. Connect to shared memory for real-time stats.");
            
            refresh();
            usleep(500000); // Update every 500ms
        }
    }
    
    endwin();
    printf("Monitor stopped.\n");
    return 0;
}

