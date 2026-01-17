#define _POSIX_C_SOURCE 200809L
#include "process_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h>

#define MAX_PROCESSES 1024
#define UPDATE_INTERVAL_MS 1000

static int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

struct monitored_process {
    struct process_info current;
    struct process_info previous;
    double user_cpu;
    double sys_cpu;
    int fd_count;
    int valid;
};

static int compare_by_cpu(const void *a, const void *b) {
    const struct monitored_process *pa = (const struct monitored_process *)a;
    const struct monitored_process *pb = (const struct monitored_process *)b;
    
    if (!pa->valid) return 1;
    if (!pb->valid) return -1;
    
    double cpu_a = pa->user_cpu + pa->sys_cpu;
    double cpu_b = pb->user_cpu + pb->sys_cpu;
    
    if (cpu_a > cpu_b) return -1;
    if (cpu_a < cpu_b) return 1;
    return 0;
}

void format_memory(unsigned long bytes, char *buf, size_t size) {
    if (bytes >= 1024UL * 1024 * 1024) {
        snprintf(buf, size, "%5.1fG", bytes / (1024.0 * 1024 * 1024));
    } else if (bytes >= 1024UL * 1024) {
        snprintf(buf, size, "%5.1fM", bytes / (1024.0 * 1024));
    } else if (bytes >= 1024) {
        snprintf(buf, size, "%5.1fK", bytes / 1024.0);
    } else {
        snprintf(buf, size, "%5luB", bytes);
    }
}

int main(void) {
    struct monitored_process processes[MAX_PROCESSES];
    struct timespec start_time, curr_time;
    int num_cpus;
    long clock_ticks;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    init_system_info();
    num_cpus = get_num_cpus();
    clock_ticks = get_clock_ticks();
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(UPDATE_INTERVAL_MS);
    
    /* Enable colors */
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    /* Initial read */
    pid_t *pid_list;
    int pid_count;
    
    memset(processes, 0, sizeof(processes));
    
    if (get_process_list(&pid_list, &pid_count) == 0) {
        for (int i = 0; i < pid_count && i < MAX_PROCESSES; i++) {
            parse_proc_stat(pid_list[i], &processes[i].current);
            parse_proc_status(pid_list[i], &processes[i].current);
            processes[i].previous = processes[i].current;
            processes[i].valid = 1;
        }
        free(pid_list);
    }
    
    while (running) {
        clock_gettime(CLOCK_MONOTONIC, &curr_time);
        
        long elapsed_ns = (curr_time.tv_sec - start_time.tv_sec) * 1000000000L +
                         (curr_time.tv_nsec - start_time.tv_nsec);
        unsigned long elapsed_ticks = (elapsed_ns * clock_ticks) / 1000000000L;
        
        /* Read current process stats */
        if (get_process_list(&pid_list, &pid_count) == 0) {
            for (int i = 0; i < MAX_PROCESSES; i++) {
                processes[i].valid = 0;
            }
            
            for (int i = 0; i < pid_count && i < MAX_PROCESSES; i++) {
                pid_t pid = pid_list[i];
                
                /* Find existing process */
                int idx = -1;
                for (int j = 0; j < MAX_PROCESSES; j++) {
                    if (processes[j].current.pid == pid) {
                        idx = j;
                        break;
                    }
                }
                
                if (idx == -1) {
                    /* New process */
                    for (int j = 0; j < MAX_PROCESSES; j++) {
                        if (!processes[j].valid) {
                            idx = j;
                            break;
                        }
                    }
                }
                
                if (idx != -1) {
                    processes[idx].previous = processes[idx].current;
                    
                    if (parse_proc_stat(pid, &processes[idx].current) == 0 &&
                        parse_proc_status(pid, &processes[idx].current) == 0) {
                        
                        processes[idx].fd_count = get_fd_count(pid);
                        
                        if (elapsed_ticks > 0) {
                            calculate_cpu_usage(&processes[idx].previous,
                                              &processes[idx].current,
                                              elapsed_ticks,
                                              num_cpus,
                                              &processes[idx].user_cpu,
                                              &processes[idx].sys_cpu);
                        }
                        
                        processes[idx].valid = 1;
                    }
                }
            }
            free(pid_list);
        }
        
        /* Sort by CPU usage */
        qsort(processes, MAX_PROCESSES, sizeof(struct monitored_process), compare_by_cpu);
        
        /* Display */
        clear();
        
        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(0, 0, "=== Process Monitor (Press Ctrl+C to exit) ===");
        attroff(COLOR_PAIR(4) | A_BOLD);
        
        mvprintw(1, 0, "System: %d CPUs | Clock: %ld ticks/sec | Elapsed: %.1fs",
                 num_cpus, clock_ticks, elapsed_ns / 1000000000.0);
        
        attron(A_BOLD);
        mvprintw(3, 0, "  PID");
        mvprintw(3, 8, "STATE");
        mvprintw(3, 15, "USER%%");
        mvprintw(3, 23, "SYS%%");
        mvprintw(3, 30, "TOTAL%%");
        mvprintw(3, 39, "THREADS");
        mvprintw(3, 49, "VSZ");
        mvprintw(3, 57, "RSS");
        mvprintw(3, 65, "FD");
        mvprintw(3, 70, "COMMAND");
        attroff(A_BOLD);
        
        int row = 4;
        int max_rows = LINES - 5;
        
        for (int i = 0; i < MAX_PROCESSES && row < max_rows; i++) {
            if (!processes[i].valid) continue;
            
            struct monitored_process *p = &processes[i];
            double total_cpu = p->user_cpu + p->sys_cpu;
            
            /* Color code by CPU usage */
            if (total_cpu > 80.0) {
                attron(COLOR_PAIR(3)); /* Red for high CPU */
            } else if (total_cpu > 20.0) {
                attron(COLOR_PAIR(2)); /* Yellow for medium CPU */
            } else {
                attron(COLOR_PAIR(1)); /* Green for low CPU */
            }
            
            mvprintw(row, 0, "%5d", p->current.pid);
            mvprintw(row, 8, "  %c  ", p->current.state);
            mvprintw(row, 15, "%5.1f", p->user_cpu);
            mvprintw(row, 23, "%5.1f", p->sys_cpu);
            mvprintw(row, 30, "%6.1f", total_cpu);
            mvprintw(row, 39, "%5d", p->current.num_threads);
            
            char vsize_buf[16], rss_buf[16];
            format_memory(p->current.vsize, vsize_buf, sizeof(vsize_buf));
            format_memory(p->current.rss, rss_buf, sizeof(rss_buf));
            
            mvprintw(row, 49, "%s", vsize_buf);
            mvprintw(row, 57, "%s", rss_buf);
            mvprintw(row, 65, "%3d", p->fd_count);
            mvprintw(row, 70, "%.30s", p->current.comm);
            
            if (total_cpu > 80.0) {
                attroff(COLOR_PAIR(3));
            } else if (total_cpu > 20.0) {
                attroff(COLOR_PAIR(2));
            } else {
                attroff(COLOR_PAIR(1));
            }
            
            row++;
        }
        
        attron(COLOR_PAIR(4));
        mvprintw(LINES - 1, 0, "Data from /proc filesystem | Update: %dms", UPDATE_INTERVAL_MS);
        attroff(COLOR_PAIR(4));
        
        refresh();
        
        /* Wait for next update or key press */
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            running = 0;
        }
        
        start_time = curr_time;
    }
    
    endwin();
    
    return 0;
}
