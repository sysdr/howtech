/*
 * monitor.c — Real-time THP metrics monitor (ncurses)
 *
 * Watches /proc/vmstat and /proc/meminfo for THP events.
 * Color-coded display:  green = healthy, yellow = activity, red = splits
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -D_GNU_SOURCE -o monitor monitor.c -lncurses
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ncurses.h>

#define REFRESH_MS      500
#define MAX_METRICS     20
#define HISTORY_LEN     60

typedef struct {
    char  name[64];
    long  value;
    long  delta;
    long  history[HISTORY_LEN];
    int   hist_pos;
} Metric;

static volatile int g_running = 1;

static void sig_handler(int sig) { (void)sig; g_running = 0; }

/* Read a single value from /proc/vmstat by key */
static long read_vmstat(const char *key)
{
    FILE *fp = fopen("/proc/vmstat", "r");
    if (!fp) return -1;
    char line[256], name[64];
    long val = -1;
    while (fgets(line, sizeof(line), fp)) {
        long v = 0;
        if (sscanf(line, "%63s %ld", name, &v) == 2) {
            if (strcmp(name, key) == 0) { val = v; break; }
        }
    }
    fclose(fp);
    return val;
}

/* Read AnonHugePages from /proc/meminfo in kB */
static long read_anon_huge_kb(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;
    char line[256];
    long val = -1;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "AnonHugePages: %ld kB", &val) == 1) break;
    }
    fclose(fp);
    return val;
}

/* Read a sysfs file into buf, return 0 on success */
static int read_sysfs(const char *path, char *buf, size_t len)
{
    FILE *fp = fopen(path, "r");
    if (!fp) { snprintf(buf, len, "N/A"); return -1; }
    if (!fgets(buf, (int)len, fp)) { snprintf(buf, len, "N/A"); }
    fclose(fp);
    /* strip newline and brackets (e.g. "[always] madvise never") */
    char *open_br = strchr(buf, '[');
    char *close_br = strchr(buf, ']');
    if (open_br && close_br) {
        size_t n = (size_t)(close_br - open_br - 1);
        memmove(buf, open_br + 1, n);
        buf[n] = '\0';
    } else {
        buf[strcspn(buf, "\n")] = '\0';
    }
    return 0;
}

/* Draw a mini sparkline bar using block chars */
static void draw_sparkline(WINDOW *win, int y, int x, Metric *m, int width)
{
    long max_val = 1;
    for (int i = 0; i < HISTORY_LEN; i++)
        if (m->history[i] > max_val) max_val = m->history[i];

    int slots = (width < HISTORY_LEN) ? width : HISTORY_LEN;
    int start = (m->hist_pos - slots + HISTORY_LEN) % HISTORY_LEN;

    for (int i = 0; i < slots; i++) {
        int idx = (start + i) % HISTORY_LEN;
        long v = m->history[idx];
        int bar = (int)(8.0 * v / max_val);
        const char *blocks[] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
        if (bar < 0) bar = 0;
        if (bar > 8) bar = 8;
        mvwaddstr(win, y, x + i, blocks[bar]);
    }
}

int main(void)
{
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    /* Metric definitions */
    const char *metric_keys[] = {
        "thp_fault_alloc",
        "thp_fault_fallback",
        "thp_fault_fallback_charge",
        "thp_collapse_alloc",
        "thp_collapse_alloc_failed",
        "thp_split_page",
        "thp_split_pmd",
        "thp_zero_page_alloc",
        NULL
    };

    int n_metrics = 0;
    Metric metrics[MAX_METRICS];
    memset(metrics, 0, sizeof(metrics));
    while (metric_keys[n_metrics] && n_metrics < MAX_METRICS) {
        strncpy(metrics[n_metrics].name, metric_keys[n_metrics],
                sizeof(metrics[0].name) - 1);
        metrics[n_metrics].value = read_vmstat(metric_keys[n_metrics]);
        n_metrics++;
    }

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_CYAN,    COLOR_BLACK);  /* title */
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);  /* healthy / alloc */
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);  /* activity */
    init_pair(4, COLOR_RED,     COLOR_BLACK);  /* splits / errors */
    init_pair(5, COLOR_WHITE,   COLOR_BLACK);  /* normal */
    init_pair(6, COLOR_BLUE,    COLOR_BLACK);  /* dim labels */
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);  /* sparkline */

    int max_rows, max_cols;

    while (g_running) {
        getmaxyx(stdscr, max_rows, max_cols);
        erase();

        /* ---- Title bar ---- */
        attron(COLOR_PAIR(1) | A_BOLD);
        mvhline(0, 0, ' ', max_cols);
        mvprintw(0, 2, " THP Live Monitor — /proc/vmstat  |  Ctrl+C to exit");
        attroff(COLOR_PAIR(1) | A_BOLD);

        /* ---- THP config line ---- */
        char thp_enabled[64], thp_defrag[64];
        read_sysfs("/sys/kernel/mm/transparent_hugepage/enabled", thp_enabled, sizeof(thp_enabled));
        read_sysfs("/sys/kernel/mm/transparent_hugepage/defrag",  thp_defrag,  sizeof(thp_defrag));

        attron(COLOR_PAIR(6));
        mvprintw(1, 2, "THP policy: enabled=[%s]  defrag=[%s]", thp_enabled, thp_defrag);
        attroff(COLOR_PAIR(6));

        /* ---- AnonHugePages ---- */
        long anon_huge = read_anon_huge_kb();
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(2, 2, "AnonHugePages: %ld MB  (%ld KB)",
                 anon_huge / 1024, anon_huge);
        attroff(COLOR_PAIR(2) | A_BOLD);

        /* ---- Column headers ---- */
        attron(COLOR_PAIR(6) | A_UNDERLINE);
        mvprintw(4, 2,  "%-42s", "Counter");
        mvprintw(4, 46, "%12s", "Cumulative");
        mvprintw(4, 60, "%8s", "Delta/s");
        mvprintw(4, 70, "  Sparkline (60s)");
        attroff(COLOR_PAIR(6) | A_UNDERLINE);

        /* ---- Update and display metrics ---- */
        for (int i = 0; i < n_metrics; i++) {
            long new_val = read_vmstat(metrics[i].name);
            long delta = 0;
            if (new_val >= 0 && metrics[i].value >= 0)
                delta = new_val - metrics[i].value;
            if (new_val >= 0) metrics[i].value = new_val;
            /* delta per refresh → approximate rate */
            metrics[i].delta = delta * (1000 / REFRESH_MS);
            /* store in history ring */
            metrics[i].history[metrics[i].hist_pos] = delta;
            metrics[i].hist_pos = (metrics[i].hist_pos + 1) % HISTORY_LEN;

            int row = 5 + i;
            if (row >= max_rows - 2) break;

            /* Color: red for splits, green for allocs, yellow for fallback */
            int color = COLOR_PAIR(5);
            if (strstr(metrics[i].name, "split"))    color = COLOR_PAIR(4);
            else if (strstr(metrics[i].name, "alloc") &&
                     !strstr(metrics[i].name, "fail")) color = COLOR_PAIR(2);
            else if (strstr(metrics[i].name, "fall")) color = COLOR_PAIR(3);

            attron(color);
            mvprintw(row, 2, "%-42s", metrics[i].name);
            mvprintw(row, 46, "%12ld", metrics[i].value);
            if (delta > 0) {
                attron(A_BOLD);
                mvprintw(row, 60, "%+8ld", delta);
                attroff(A_BOLD);
            } else {
                mvprintw(row, 60, "%8s", ".");
            }
            attroff(color);

            /* Sparkline */
            attron(COLOR_PAIR(7));
            int spark_width = max_cols - 70 - 2;
            if (spark_width > 2)
                draw_sparkline(stdscr, row, 70, &metrics[i], spark_width);
            attroff(COLOR_PAIR(7));
        }

        /* ---- Status bar ---- */
        attron(COLOR_PAIR(6));
        mvhline(max_rows - 1, 0, ' ', max_cols);
        mvprintw(max_rows - 1, 2,
                 "Refresh: %dms | green=healthy alloc | yellow=fallback | red=splits (CoW indicator)",
                 REFRESH_MS);
        attroff(COLOR_PAIR(6));

        /* ---- Time ---- */
        time_t t = time(NULL);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", localtime(&t));
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, max_cols - 12, " %s ", tbuf);
        attroff(COLOR_PAIR(1) | A_BOLD);

        refresh();

        /* Sleep in small chunks to remain responsive to input */
        for (int ms = 0; ms < REFRESH_MS && g_running; ms += 50) {
            int ch = getch();
            if (ch == 'q' || ch == 'Q') { g_running = 0; break; }
            struct timespec ts = { 0, 50 * 1000 * 1000L };
            nanosleep(&ts, NULL);
        }
    }

    endwin();
    printf("\nMonitor exited.\n");
    return 0;
}
