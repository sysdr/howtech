#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>
#include <sys/sysinfo.h>

#define MAX_CPUS 128
#define MAX_IRQS 256
#define UPDATE_INTERVAL_MS 500

typedef struct {
    char name[32];
    unsigned long long counts[MAX_CPUS];
    unsigned long long prev_counts[MAX_CPUS];
    unsigned long long total;
    unsigned long long prev_total;
} irq_stat_t;

typedef struct {
    unsigned long long counts[10];  // HI, TIMER, NET_TX, NET_RX, BLOCK, IRQ_POLL, TASKLET, SCHED, HRTIMER, RCU
    unsigned long long prev_counts[10];
} softirq_stat_t;

static irq_stat_t irqs[MAX_IRQS];
static softirq_stat_t softirqs[MAX_CPUS];
static int num_cpus;
static int num_irqs;

static const char *softirq_names[] = {
    "HI", "TIMER", "NET_TX", "NET_RX", "BLOCK",
    "IRQ_POLL", "TASKLET", "SCHED", "HRTIMER", "RCU"
};

void read_interrupts(void) {
    FILE *fp = fopen("/proc/interrupts", "r");
    if (!fp) return;
    
    char line[1024];
    if (fgets(line, sizeof(line), fp) == NULL) { // Skip header
        fclose(fp);
        return;
    }
    
    num_irqs = 0;
    while (fgets(line, sizeof(line), fp) && num_irqs < MAX_IRQS) {
        char *colon = strchr(line, ':');
        if (!colon) continue;
        
        *colon = '\0';
        // IRQ number is at start of line, but we don't need it
        
        irq_stat_t *irq = &irqs[num_irqs];
        memcpy(irq->prev_counts, irq->counts, sizeof(irq->counts));
        irq->prev_total = irq->total;
        irq->total = 0;
        
        char *ptr = colon + 1;
        for (int cpu = 0; cpu < num_cpus; cpu++) {
            unsigned long long count = strtoull(ptr, &ptr, 10);
            irq->counts[cpu] = count;
            irq->total += count;
        }
        
        // Extract IRQ name
        while (*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;
        char *name_start = ptr;
        while (*ptr && *ptr != '\n') ptr++;
        *ptr = '\0';
        
        // Simplify name
        char *device = strstr(name_start, "  ");
        if (device) {
            device += 2;
            while (*device == ' ') device++;
            strncpy(irq->name, device, sizeof(irq->name) - 1);
        } else {
            strncpy(irq->name, name_start, sizeof(irq->name) - 1);
        }
        irq->name[sizeof(irq->name) - 1] = '\0';
        
        num_irqs++;
    }
    
    fclose(fp);
}

void read_softirqs(void) {
    FILE *fp = fopen("/proc/softirqs", "r");
    if (!fp) return;
    
    char line[1024];
    if (fgets(line, sizeof(line), fp) == NULL) { // Skip header
        fclose(fp);
        return;
    }
    
    for (int i = 0; i < 10; i++) {
        if (!fgets(line, sizeof(line), fp)) break;
        
        char *colon = strchr(line, ':');
        if (!colon) continue;
        
        char *ptr = colon + 1;
        for (int cpu = 0; cpu < num_cpus; cpu++) {
            softirq_stat_t *s = &softirqs[cpu];
            s->prev_counts[i] = s->counts[i];
            s->counts[i] = strtoull(ptr, &ptr, 10);
        }
    }
    
    fclose(fp);
}

void draw_header(WINDOW *win) {
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, 0, 1, "IRQ HANDLER EXECUTION MONITOR");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 0, COLS - 20, "Time: %s", time_str);
    wattroff(win, COLOR_PAIR(3));
    
    mvwhline(win, 1, 0, ACS_HLINE, COLS);
}

void draw_cpu_info(WINDOW *win, int start_y) {
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, start_y, 1, "CPU CORES: %d", num_cpus);
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    
    struct sysinfo si;
    sysinfo(&si);
    
    mvwprintw(win, start_y + 1, 1, "Uptime: %ld days, %ld hours", 
              si.uptime / 86400, (si.uptime % 86400) / 3600);
    mvwprintw(win, start_y + 2, 1, "Load: %.2f, %.2f, %.2f",
              si.loads[0] / 65536.0, si.loads[1] / 65536.0, si.loads[2] / 65536.0);
}

void draw_top_irqs(WINDOW *win, int start_y) {
    mvwhline(win, start_y, 0, ACS_HLINE, COLS);
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, start_y + 1, 1, "TOP HARDWARE INTERRUPTS (by rate)");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    mvwprintw(win, start_y + 2, 1, "%-20s %12s %15s", "IRQ Device", "Total Count", "Rate (per sec)");
    mvwhline(win, start_y + 3, 0, ACS_HLINE, COLS);
    
    int y = start_y + 4;
    int displayed = 0;
    
    for (int i = 0; i < num_irqs && displayed < 10; i++) {
        irq_stat_t *irq = &irqs[i];
        unsigned long long delta = irq->total - irq->prev_total;
        unsigned long long rate = delta * 1000 / UPDATE_INTERVAL_MS;
        
        if (rate > 0) {
            wattron(win, COLOR_PAIR(rate > 1000 ? 4 : 2));
            mvwprintw(win, y++, 1, "%-20.20s %12llu %15llu", 
                      irq->name, irq->total, rate);
            wattroff(win, COLOR_PAIR(rate > 1000 ? 4 : 2));
            displayed++;
        }
    }
}

void draw_softirqs(WINDOW *win, int start_y) {
    mvwhline(win, start_y, 0, ACS_HLINE, COLS);
    wattron(win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(win, start_y + 1, 1, "SOFTIRQ PROCESSING (Bottom-Half Work)");
    wattroff(win, COLOR_PAIR(1) | A_BOLD);
    
    mvwprintw(win, start_y + 2, 1, "%-12s", "Type");
    for (int cpu = 0; cpu < num_cpus && cpu < 8; cpu++) {
        mvwprintw(win, start_y + 2, 14 + cpu * 10, "CPU%-2d", cpu);
    }
    mvwhline(win, start_y + 3, 0, ACS_HLINE, COLS);
    
    int y = start_y + 4;
    for (int i = 0; i < 10; i++) {
        wattron(win, COLOR_PAIR(3));
        mvwprintw(win, y, 1, "%-12s", softirq_names[i]);
        wattroff(win, COLOR_PAIR(3));
        
        for (int cpu = 0; cpu < num_cpus && cpu < 8; cpu++) {
            softirq_stat_t *s = &softirqs[cpu];
            unsigned long long delta = s->counts[i] - s->prev_counts[i];
            unsigned long long rate = delta * 1000 / UPDATE_INTERVAL_MS;
            
            if (rate > 0) {
                wattron(win, COLOR_PAIR(rate > 100 ? 4 : 2));
                mvwprintw(win, y, 14 + cpu * 10, "%8llu", rate);
                wattroff(win, COLOR_PAIR(rate > 100 ? 4 : 2));
            }
        }
        y++;
    }
}

void draw_legend(WINDOW *win, int start_y) {
    mvwhline(win, start_y, 0, ACS_HLINE, COLS);
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, start_y + 1, 1, "LEGEND:");
    wattroff(win, COLOR_PAIR(1));
    
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, start_y + 1, 10, "Normal rate");
    wattroff(win, COLOR_PAIR(2));
    
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, start_y + 1, 30, "High rate");
    wattroff(win, COLOR_PAIR(4));
    
    mvwprintw(win, start_y + 1, 50, "Press 'q' to quit");
}

int main(void) {
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus > MAX_CPUS) num_cpus = MAX_CPUS;
    
    // Check if we have permission to read /proc
    if (access("/proc/interrupts", R_OK) != 0) {
        fprintf(stderr, "Error: Cannot read /proc/interrupts. Run with appropriate permissions.\n");
        return 1;
    }
    
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    
    while (1) {
        clear();
        
        read_interrupts();
        read_softirqs();
        
        draw_header(stdscr);
        draw_cpu_info(stdscr, 3);
        draw_top_irqs(stdscr, 7);
        draw_softirqs(stdscr, 21);
        draw_legend(stdscr, LINES - 3);
        
        refresh();
        
        // Check for quit
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
        
        usleep(UPDATE_INTERVAL_MS * 1000);
    }
    
    endwin();
    return 0;
}
