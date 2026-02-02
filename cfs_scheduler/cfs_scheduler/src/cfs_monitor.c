#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <ncurses.h>
#include <time.h>
#include <errno.h>

#define MAX_TASKS 1024
#define REFRESH_MS 500

typedef struct {
    pid_t pid;
    char name[64];
    unsigned long long vruntime;
    unsigned long long sum_exec_runtime;
    unsigned long nr_switches;
    int prio;
    long nice;
} task_sched_t;

int read_sched_stat(pid_t pid, task_sched_t *task) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/sched", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    task->pid = pid;
    task->vruntime = 0;
    task->sum_exec_runtime = 0;
    task->nr_switches = 0;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%63s", task->name) == 1) {
            // First line is process name
            continue;
        }
        
        if (strstr(line, "se.vruntime")) {
            sscanf(line, " se.vruntime : %llu", &task->vruntime);
        } else if (strstr(line, "se.sum_exec_runtime")) {
            sscanf(line, " se.sum_exec_runtime : %llu", &task->sum_exec_runtime);
        } else if (strstr(line, "nr_switches")) {
            sscanf(line, " nr_switches : %lu", &task->nr_switches);
        } else if (strstr(line, "prio")) {
            sscanf(line, " prio : %d", &task->prio);
        }
    }
    
    fclose(f);
    
    // Read nice value from stat
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (f) {
        char stat_line[1024];
        if (fgets(stat_line, sizeof(stat_line), f)) {
            // nice is 19th field in /proc/pid/stat
            char *token = stat_line;
            for (int i = 0; i < 18; i++) {
                token = strchr(token + 1, ' ');
                if (!token) break;
            }
            if (token) {
                sscanf(token, "%ld", &task->nice);
            }
        }
        fclose(f);
    }
    
    return 0;
}

int find_interesting_tasks(task_sched_t tasks[], int max_tasks) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir /proc");
        return 0;
    }
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(proc_dir)) && count < max_tasks) {
        // Check if directory name is a number (PID)
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        task_sched_t task;
        if (read_sched_stat(pid, &task) == 0) {
            // Filter for CPU-intensive tasks or our demo tasks
            if (task.sum_exec_runtime > 100000000 || // > 100ms runtime
                strstr(task.name, "cfs_demo") ||
                strstr(task.name, "HIGH_PRIO") ||
                strstr(task.name, "NORMAL") ||
                strstr(task.name, "LOW_PRIO") ||
                strstr(task.name, "VERY_LOW")) {
                tasks[count++] = task;
            }
        }
    }
    
    closedir(proc_dir);
    return count;
}

int compare_vruntime(const void *a, const void *b) {
    const task_sched_t *ta = (const task_sched_t *)a;
    const task_sched_t *tb = (const task_sched_t *)b;
    
    if (ta->vruntime < tb->vruntime) return -1;
    if (ta->vruntime > tb->vruntime) return 1;
    return 0;
}

void display_tasks(WINDOW *win, task_sched_t tasks[], int count) {
    wclear(win);
    box(win, 0, 0);
    
    mvwprintw(win, 1, 2, "CFS Scheduler Monitor - Red-Black Tree Visualization");
    mvwprintw(win, 2, 2, "Tasks sorted by vruntime (leftmost = next to run)");
    wattron(win, A_BOLD);
    mvwprintw(win, 4, 2, "%-20s %8s %8s %15s %15s %10s", 
              "TASK", "PID", "NICE", "VRUNTIME", "EXEC_TIME(ms)", "SWITCHES");
    wattroff(win, A_BOLD);
    
    int row = 5;
    for (int i = 0; i < count && row < LINES - 3; i++) {
        task_sched_t *t = &tasks[i];
        
        // Highlight leftmost (lowest vruntime)
        if (i == 0) wattron(win, COLOR_PAIR(1) | A_BOLD);
        
        mvwprintw(win, row++, 2, "%-20s %8d %8ld %15llu %15llu %10lu",
                  t->name, t->pid, t->nice, t->vruntime, 
                  t->sum_exec_runtime / 1000000, // Convert to ms
                  t->nr_switches);
        
        if (i == 0) wattroff(win, COLOR_PAIR(1) | A_BOLD);
    }
    
    mvwprintw(win, LINES - 2, 2, "Press 'q' to quit | Refresh: %dms", REFRESH_MS);
    
    wrefresh(win);
}

int main(void) {
    // Initialize ncurses
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
    }
    
    WINDOW *main_win = newwin(LINES, COLS, 0, 0);
    keypad(main_win, TRUE);
    
    task_sched_t tasks[MAX_TASKS];
    
    while (1) {
        int count = find_interesting_tasks(tasks, MAX_TASKS);
        
        // Sort by vruntime to simulate red-black tree ordering
        qsort(tasks, count, sizeof(task_sched_t), compare_vruntime);
        
        display_tasks(main_win, tasks, count);
        
        // Check for 'q' key
        int ch = wgetch(main_win);
        if (ch == 'q' || ch == 'Q') break;
        
        napms(REFRESH_MS);
    }
    
    delwin(main_win);
    endwin();
    
    return 0;
}
