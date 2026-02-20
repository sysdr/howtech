/*
 * monitor.c - Real-time memory allocator stats monitor (ncurses)
 * Compile: gcc -Wall -Wextra -Werror -O2 -o monitor monitor.c -lncurses
 *
 * Watches /proc/PID/status, smaps_rollup, oom_score in real time.
 * Run alongside alloc_demo or any target process.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#define HISTORY_LEN 40

static volatile sig_atomic_t g_quit = 0;

static void handle_sig(int s) { (void)s; g_quit = 1; }

typedef struct {
    pid_t  pid;
    char   name[64];
    long   rss_kb;
    long   vsz_kb;
    long   anon_kb;
    long   shared_kb;
    int    oom_score;
    int    oom_adj;
} ProcInfo;

static int read_proc(pid_t pid, ProcInfo *out)
{
    char path[128];
    char line[512];
    FILE *f;

    memset(out, 0, sizeof(*out));
    out->pid = pid;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) return -1;
    while (fgets(line, sizeof(line), f)) {
        if      (strncmp(line, "Name:", 5) == 0)
            sscanf(line + 5, " %63s", out->name);
        else if (strncmp(line, "VmRSS:", 6) == 0)
            sscanf(line + 6, "%ld", &out->rss_kb);
        else if (strncmp(line, "VmSize:", 7) == 0)
            sscanf(line + 7, "%ld", &out->vsz_kb);
        else if (strncmp(line, "VmRSS:", 6) == 0)
            sscanf(line + 6, "%ld", &out->rss_kb);
    }
    fclose(f);

    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            long val = 0;
            if      (strncmp(line, "Anonymous:", 10) == 0 && sscanf(line+10, "%ld", &val)==1)
                out->anon_kb += val;
            else if (strncmp(line, "Shared_Clean:", 13) == 0 && sscanf(line+13, "%ld", &val)==1)
                out->shared_kb += val;
            else if (strncmp(line, "Shared_Dirty:", 13) == 0 && sscanf(line+13, "%ld", &val)==1)
                out->shared_kb += val;
        }
        fclose(f);
    }

    snprintf(path, sizeof(path), "/proc/%d/oom_score", pid);
    f = fopen(path, "r");
    if (f) {
        if (fscanf(f, "%d", &out->oom_score) != 1)
            out->oom_score = 0;
        fclose(f);
    }

    snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);
    f = fopen(path, "r");
    if (f) {
        if (fscanf(f, "%d", &out->oom_adj) != 1)
            out->oom_adj = 0;
        fclose(f);
    }

    return 0;
}

static long get_mem_total(void)
{
    static long cached = 0;
    if (cached) return cached;
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 1;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%ld", &cached);
            break;
        }
    }
    fclose(f);
    return cached ? cached : 1;
}

static void read_pressure(float *mem_some, float *mem_full)
{
    *mem_some = 0.0f; *mem_full = 0.0f;
    FILE *f = fopen("/proc/pressure/memory", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "some", 4) == 0)
            sscanf(line, "some avg10=%f", mem_some);
        else if (strncmp(line, "full", 4) == 0)
            sscanf(line, "full avg10=%f", mem_full);
    }
    fclose(f);
}

/* Collect top-N processes by RSS */
#define MAX_PROCS 8
static int collect_top_procs(ProcInfo procs[MAX_PROCS], int *cnt)
{
    *cnt = 0;
    DIR *dp = opendir("/proc");
    if (!dp) return -1;

    struct dirent *ent;
    ProcInfo tmp[512];
    int n = 0;

    while ((ent = readdir(dp)) != NULL && n < 512) {
        pid_t pid = (pid_t)atoi(ent->d_name);
        if (pid <= 0) continue;
        if (read_proc(pid, &tmp[n]) == 0)
            n++;
    }
    closedir(dp);

    /* Sort by RSS descending (simple insertion sort) */
    for (int i = 1; i < n; i++) {
        ProcInfo key = tmp[i];
        int j = i - 1;
        while (j >= 0 && tmp[j].rss_kb < key.rss_kb) {
            tmp[j+1] = tmp[j];
            j--;
        }
        tmp[j+1] = key;
    }

    *cnt = (n < MAX_PROCS) ? n : MAX_PROCS;
    for (int i = 0; i < *cnt; i++)
        procs[i] = tmp[i];

    return 0;
}

/* Draw a bar [████░░░░] width w, filled pct 0-100 */
static void draw_bar(WINDOW *win, int y, int x, int w, int pct, int color_pair)
{
    int filled = (pct * w) / 100;
    wattron(win, COLOR_PAIR(color_pair));
    for (int i = 0; i < w; i++) {
        mvwaddch(win, y, x + i, (i < filled) ? ACS_CKBOARD : '.');
    }
    wattroff(win, COLOR_PAIR(color_pair));
}

int main(int argc, char *argv[])
{
    pid_t watch_pid = -1;
    if (argc >= 2)
        watch_pid = (pid_t)atoi(argv[1]);

    signal(SIGINT,  handle_sig);
    signal(SIGTERM, handle_sig);

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    start_color();

    /* Color pairs */
    init_pair(1, COLOR_CYAN,    COLOR_BLACK);  /* title     */
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);  /* good      */
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);  /* warning   */
    init_pair(4, COLOR_RED,     COLOR_BLACK);  /* critical  */
    init_pair(5, COLOR_WHITE,   COLOR_BLACK);  /* normal    */
    init_pair(6, COLOR_BLUE,    COLOR_BLACK);  /* bars      */
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);  /* highlight */

    long rss_history[HISTORY_LEN];
    memset(rss_history, 0, sizeof(rss_history));

    ProcInfo self_info;
    memset(&self_info, 0, sizeof(self_info));

    int frame = 0;

    while (!g_quit) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        (void)rows;
        clear();

        /* ── Title bar ───────────────────────── */
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(0, 0, "%-*s", cols, "  Memory Allocator Monitor — Systems Programming Deep Dive");
        attroff(COLOR_PAIR(1) | A_BOLD);

        time_t now = time(NULL);
        char tbuf[32];
        struct tm *tm_info = localtime(&now);
        strftime(tbuf, sizeof(tbuf), "%H:%M:%S", tm_info);
        attron(COLOR_PAIR(5) | A_DIM);
        mvprintw(0, cols - 12, "[%s]", tbuf);
        attroff(COLOR_PAIR(5) | A_DIM);

        int row = 1;

        /* ── Memory Pressure (PSI) ───────────── */
        float psi_some = 0, psi_full = 0;
        read_pressure(&psi_some, &psi_full);

        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(row, 2, "Memory Pressure (PSI)");
        attroff(COLOR_PAIR(5) | A_BOLD);
        row++;

        mvprintw(row, 4, "some avg10: %5.1f%%  ", psi_some);
        draw_bar(stdscr, row, 26, 30, (int)psi_some, psi_some > 5.0f ? 4 : 2);
        if (psi_some > 5.0f) {
            attron(COLOR_PAIR(4) | A_BOLD);
            mvprintw(row, 58, " ⚠ HIGH — shed load!");
            attroff(COLOR_PAIR(4) | A_BOLD);
        }
        row++;

        mvprintw(row, 4, "full avg10: %5.1f%%  ", psi_full);
        draw_bar(stdscr, row, 26, 30, (int)psi_full, psi_full > 1.0f ? 4 : 2);
        row += 2;

        /* ── Self or watched PID ─────────────── */
        pid_t target = (watch_pid > 0) ? watch_pid : getpid();
        read_proc(target, &self_info);

        long mem_total = get_mem_total();
        int  rss_pct   = (int)(self_info.rss_kb * 100 / mem_total);

        /* Rolling RSS history */
        memmove(&rss_history[0], &rss_history[1], (HISTORY_LEN - 1) * sizeof(long));
        rss_history[HISTORY_LEN - 1] = self_info.rss_kb;

        attron(COLOR_PAIR(7) | A_BOLD);
        mvprintw(row, 2, "Watched Process: PID=%-6d (%s)", target, self_info.name);
        attroff(COLOR_PAIR(7) | A_BOLD);
        row++;

        mvprintw(row, 4, "RSS:     %8ld KB  (%3d%% of RAM)  ",
                 self_info.rss_kb, rss_pct);
        draw_bar(stdscr, row, 42, 28, rss_pct, rss_pct > 50 ? 4 : (rss_pct > 20 ? 3 : 2));
        row++;

        mvprintw(row, 4, "VSZ:     %8ld KB  (virtual)", self_info.vsz_kb);
        row++;
        mvprintw(row, 4, "Anon:    %8ld KB  (private mappings)", self_info.anon_kb);
        row++;
        mvprintw(row, 4, "Shared:  %8ld KB  (shared libs/files)", self_info.shared_kb);
        row++;

        /* Fragmentation estimate: anon vs rss gap */
        long gap = self_info.rss_kb - self_info.anon_kb - self_info.shared_kb;
        if (gap < 0) gap = 0;
        mvprintw(row, 4, "Overhead:%8ld KB  (mapped files, stack, etc.)", gap);
        row += 2;

        /* OOM score */
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(row, 2, "OOM Killer Status");
        attroff(COLOR_PAIR(5) | A_BOLD);
        row++;

        int oom_pair = (self_info.oom_score > 700) ? 4
                     : (self_info.oom_score > 400) ? 3 : 2;
        attron(COLOR_PAIR(oom_pair));
        mvprintw(row, 4, "oom_score:     %d", self_info.oom_score);
        attroff(COLOR_PAIR(oom_pair));
        mvprintw(row, 30, "(adj: %+d)", self_info.oom_adj);
        row++;
        mvprintw(row, 4, "kill risk:     ");
        draw_bar(stdscr, row, 18, 30,
                 (self_info.oom_score > 1000 ? 100 : self_info.oom_score / 10),
                 oom_pair);
        row += 2;

        /* ── RSS sparkline ───────────────────── */
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(row, 2, "RSS History (last %d samples)", HISTORY_LEN);
        attroff(COLOR_PAIR(5) | A_BOLD);
        row++;

        long max_rss = 1;
        for (int i = 0; i < HISTORY_LEN; i++)
            if (rss_history[i] > max_rss) max_rss = rss_history[i];

        static const char *sparks[] = { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };
        attron(COLOR_PAIR(6));
        move(row, 4);
        for (int i = 0; i < HISTORY_LEN; i++) {
            int idx = (int)(rss_history[i] * 8 / max_rss);
            if (idx < 0) idx = 0;
            if (idx > 8) idx = 8;
            addstr(sparks[idx]);
        }
        attroff(COLOR_PAIR(6));
        mvprintw(row, 4 + HISTORY_LEN + 2, " max: %ld KB", max_rss);
        row += 2;

        /* ── Top processes by RSS ────────────── */
        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(row, 2, "Top Processes by RSS");
        attroff(COLOR_PAIR(5) | A_BOLD);
        row++;

        ProcInfo top[MAX_PROCS];
        int      top_cnt = 0;
        collect_top_procs(top, &top_cnt);

        attron(COLOR_PAIR(5) | A_DIM);
        mvprintw(row, 4, "%-6s %-18s %8s %8s %6s %6s",
                 "PID", "NAME", "RSS KB", "VSZ KB", "OOM", "OOM_ADJ");
        attroff(COLOR_PAIR(5) | A_DIM);
        row++;

        for (int i = 0; i < top_cnt && row < rows - 3; i++, row++) {
            int pair = (top[i].oom_score > 500) ? 4 : 5;
            attron(COLOR_PAIR(pair));
            mvprintw(row, 4, "%-6d %-18.18s %8ld %8ld %6d %6d",
                     top[i].pid, top[i].name,
                     top[i].rss_kb, top[i].vsz_kb,
                     top[i].oom_score, top[i].oom_adj);
            attroff(COLOR_PAIR(pair));
        }

        /* ── Footer ──────────────────────────── */
        attron(COLOR_PAIR(5) | A_DIM);
        mvprintw(rows - 1, 0, " [q] quit   frame: %d   refresh: 1s", ++frame);
        attroff(COLOR_PAIR(5) | A_DIM);

        refresh();
        usleep(1000000); /* 1 second refresh */
    }

    endwin();
    printf("\nMonitor exited.\n");
    return 0;
}
