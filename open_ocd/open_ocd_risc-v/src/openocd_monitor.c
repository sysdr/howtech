/*
 * OpenOCD Configuration Monitor
 * Real-time display of configuration status and analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

#define CONFIG_FILE "configs/riscv_target.cfg"

typedef struct {
    int tap_count;
    int jtag_speed_khz;
    float clock_period_us;
    int ir_length;
    bool config_valid;
} monitor_state_t;

void init_monitor(void) {
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    timeout(1000);
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_BLUE);
}

void draw_box(int y, int x, int height, int width, const char *title) {
    attron(COLOR_PAIR(4));
    
    // Top border
    mvaddch(y, x, ACS_ULCORNER);
    for (int i = 1; i < width - 1; i++) mvaddch(y, x + i, ACS_HLINE);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    
    // Sides
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
    
    // Bottom border
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    for (int i = 1; i < width - 1; i++) mvaddch(y + height - 1, x + i, ACS_HLINE);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    
    // Title
    if (title) {
        attron(A_BOLD);
        mvprintw(y, x + 2, " %s ", title);
        attroff(A_BOLD);
    }
    
    attroff(COLOR_PAIR(4));
}

void update_display(monitor_state_t *state) {
    clear();
    
    // Header
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(0, 0, "%-80s", "  OpenOCD RISC-V Configuration Monitor  ");
    attroff(COLOR_PAIR(5) | A_BOLD);
    
    time_t now = time(NULL);
    mvprintw(1, 0, "Time: %s", ctime(&now));
    
    // Configuration Status
    draw_box(3, 2, 12, 76, "Configuration Status");
    
    if (state->config_valid) {
        attron(COLOR_PAIR(1));
        mvprintw(5, 4, "✓ Configuration Valid");
        attroff(COLOR_PAIR(1));
    } else {
        attron(COLOR_PAIR(3));
        mvprintw(5, 4, "✗ Configuration Invalid");
        attroff(COLOR_PAIR(3));
    }
    
    mvprintw(7, 4, "JTAG TAPs Defined:     %d", state->tap_count);
    mvprintw(8, 4, "JTAG Clock Speed:      %d kHz (%.2f MHz)", 
             state->jtag_speed_khz, state->jtag_speed_khz / 1000.0);
    mvprintw(9, 4, "Clock Period:          %.1f μs (%.0f ns)", 
             state->clock_period_us, state->clock_period_us * 1000);
    mvprintw(10, 4, "IR Length:             %d bits", state->ir_length);
    
    float ir_scan_time = state->ir_length * state->clock_period_us;
    float dmi_scan_time = 41 * state->clock_period_us;
    mvprintw(11, 4, "IR Scan Time:          %.1f μs", ir_scan_time);
    mvprintw(12, 4, "DMI Scan Time:         %.1f μs (41 bits)", dmi_scan_time);
    
    // Performance Analysis
    draw_box(15, 2, 8, 76, "Performance Analysis");
    
    float reg_read_ms = (dmi_scan_time * 3) / 1000.0;
    float mem_1kb_ms = reg_read_ms * 16;
    
    mvprintw(17, 4, "Single Register Read:  %.2f ms", reg_read_ms);
    mvprintw(18, 4, "1KB Memory Dump:       %.1f ms (estimated)", mem_1kb_ms);
    mvprintw(19, 4, "GDB Responsiveness:    %s", 
             mem_1kb_ms < 100 ? "Excellent" : mem_1kb_ms < 500 ? "Good" : "Slow");
    
    // Recommendations
    draw_box(24, 2, 6, 76, "Recommendations");
    
    int rec_line = 26;
    if (state->jtag_speed_khz > 2000) {
        attron(COLOR_PAIR(2));
        mvprintw(rec_line++, 4, "⚠ JTAG speed > 2 MHz: May cause signal integrity issues");
        attroff(COLOR_PAIR(2));
    } else if (state->jtag_speed_khz < 100) {
        attron(COLOR_PAIR(1));
        mvprintw(rec_line++, 4, "✓ Conservative JTAG speed: Good for bring-up and debugging");
        attroff(COLOR_PAIR(1));
    }
    
    if (state->tap_count == 0) {
        attron(COLOR_PAIR(3));
        mvprintw(rec_line++, 4, "✗ No JTAG TAPs defined: Check configuration file");
        attroff(COLOR_PAIR(3));
    }
    
    // Footer
    attron(COLOR_PAIR(4));
    mvprintw(LINES - 2, 2, "Press 'q' to quit");
    attroff(COLOR_PAIR(4));
    
    refresh();
}

int main(void) {
    init_monitor();
    
    monitor_state_t state = {
        .tap_count = 1,
        .jtag_speed_khz = 100,
        .ir_length = 5,
        .config_valid = true
    };
    state.clock_period_us = 1000.0 / state.jtag_speed_khz;
    
    int ch;
    while ((ch = getch()) != 'q') {
        // Simulate some variation for demonstration
        state.jtag_speed_khz = 100 + (rand() % 50);
        state.clock_period_us = 1000.0 / state.jtag_speed_khz;
        
        update_display(&state);
    }
    
    endwin();
    return 0;
}
