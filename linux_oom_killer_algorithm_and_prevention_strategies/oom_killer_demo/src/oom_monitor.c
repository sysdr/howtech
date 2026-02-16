/*
 * oom_monitor.c — Real-time OOM score monitor with ncurses UI
 *
 * Shows:
 *  - Top processes sorted by oom_score (color-coded)
 *  - PSI memory pressure bar (some / full)
 *  - System memory stats (MemAvailable, CommitLimit, Committed_AS)
 *  - Refresh every 1s; press 'q' to quit
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -g \
 *              -o bin/oom_monitor src/oom_monitor.c -lncurses
 */

#define _GNU_SOURCE
#include <ncurses.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#define MAX_PROCS  512
#define REFRESH_MS 1000

/* ── data types ──────────────────────────────────────────── */

typedef struct {
    int  pid;
    int  score;
    int  adj;
    long rss_mib;
    char name[48];
} Proc;

typedef struct {
    long mem_total;
    long mem_avail;
    long commit_limit;
    long committed;
    long swap_total;
    long swap_free;
} MemStats;

typedef struct {
    double some_avg10;
    double full_avg10;
    int    available;
} PsiStats;

/* ── globals ─────────────────────────────────────────────── */

static Proc     procs[MAX_PROCS];
static int      proc_count  = 0;
static volatile int running = 1;

/* ── signal handler ──────────────────────────────────────── */

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

/* ── helpers ─────────────────────────────────────────────── */

static int is_numeric(const char *s)
{
    if (!*s) return 0;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static int read_long(const char *path, long *out)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int ret = (fscanf(f, "%ld", out) == 1) ? 0 : -1;
    fclose(f);
    return ret;
}

/* ── proc scanner ────────────────────────────────────────── */

static void scan_procs(void)
{
    DIR *d = opendir("/proc");
    if (!d) return;

    proc_count = 0;
    struct dirent *ent;

    while ((ent = readdir(d)) != NULL && proc_count < MAX_PROCS) {
        if (!is_numeric(ent->d_name)) continue;

        int pid = atoi(ent->d_name);
        char path[128];
        long score = 0, adj = 0;

        snprintf(path, sizeof(path), "/proc/%d/oom_score", pid);
        if (read_long(path, &score) < 0) continue;

        snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);
        (void)read_long(path, &adj);

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        char name[48] = "?";
        long rss_kib  = 0;
        char line[256];

        while (fgets(line, sizeof(line), f)) {
            char key[64];
            if (sscanf(line, "%63[^:]:", key) != 1) continue;
            if (strcmp(key, "Name") == 0)
                sscanf(line, "%*[^:]: %47s", name);
            else if (strcmp(key, "VmRSS") == 0)
                sscanf(line, "%*[^:]: %ld", &rss_kib);
        }
        fclose(f);

        procs[proc_count].pid     = pid;
        procs[proc_count].score   = (int)score;
        procs[proc_count].adj     = (int)adj;
        procs[proc_count].rss_mib = rss_kib / 1024;
        memcpy(procs[proc_count].name, name, 47); procs[proc_count].name[47] = '\0';
        procs[proc_count].name[47] = '\0';
        proc_count++;
    }
    closedir(d);
}

static int cmp_score(const void *a, const void *b)
{
    return ((const Proc *)b)->score - ((const Proc *)a)->score;
}

/* ── meminfo reader ──────────────────────────────────────── */

static void read_memstats(MemStats *ms)
{
    memset(ms, 0, sizeof(*ms));
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    char line[256], key[64];
    long val;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%63[^:]: %ld", key, &val) != 2) continue;
        if      (strcmp(key, "MemTotal")     == 0) ms->mem_total    = val;
        else if (strcmp(key, "MemAvailable") == 0) ms->mem_avail    = val;
        else if (strcmp(key, "CommitLimit")  == 0) ms->commit_limit = val;
        else if (strcmp(key, "Committed_AS") == 0) ms->committed    = val;
        else if (strcmp(key, "SwapTotal")    == 0) ms->swap_total   = val;
        else if (strcmp(key, "SwapFree")     == 0) ms->swap_free    = val;
    }
    fclose(f);
}

/* ── PSI reader ──────────────────────────────────────────── */

static void read_psi(PsiStats *ps)
{
    memset(ps, 0, sizeof(*ps));
    FILE *f = fopen("/proc/pressure/memory", "r");
    if (!f) { ps->available = 0; return; }
    ps->available = 1;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        double a10 = 0.0, a60 = 0.0, a300 = 0.0;
        unsigned long long total = 0;
        if (sscanf(line, "some avg10=%lf avg60=%lf avg300=%lf total=%llu",
                   &a10, &a60, &a300, &total) == 4)
            ps->some_avg10 = a10;
        else if (sscanf(line, "full avg10=%lf avg60=%lf avg300=%lf total=%llu",
                        &a10, &a60, &a300, &total) == 4)
            ps->full_avg10 = a10;
    }
    fclose(f);
}

/* ── ncurses draw ────────────────────────────────────────── */

static void draw_bar(int row, int col, double pct, int width,
                     int color_pair)
{
    int filled = (int)(pct / 100.0 * (double)width);
    if (filled > width) filled = width;

    attron(COLOR_PAIR(color_pair));
    for (int i = 0; i < filled; i++)
        mvaddch(row, col + i, ACS_BLOCK);
    attroff(COLOR_PAIR(color_pair));
    for (int i = filled; i < width; i++)
        mvaddch(row, col + i, ACS_CKBOARD);
}

/* color pairs */
#define CP_TITLE   1   /* blue on black     */
#define CP_HEADER  2   /* cyan on black     */
#define CP_HIGH    3   /* red on black      */
#define CP_MED     4   /* yellow on black   */
#define CP_LOW     5   /* green on black    */
#define CP_NORMAL  6   /* white on black    */
#define CP_ALERT   7   /* red bold          */
#define CP_BAR_HI  8   /* red bar           */
#define CP_BAR_LO  9   /* green bar         */

static void draw_screen(const MemStats *ms, const PsiStats *ps)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows;

    clear();

    /* Title bar */
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(0, 0, "%-*s", cols,
             "  OOM Score Monitor  — Linux OOM Killer Deep Dive  (q=quit)");
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    /* Clock */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", tm_info);
    attron(COLOR_PAIR(CP_HEADER));
    mvprintw(0, cols - 10, "%s", ts);
    attroff(COLOR_PAIR(CP_HEADER));

    /* Memory stats */
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(2, 2, "SYSTEM MEMORY");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    long total_mib  = ms->mem_total    / 1024;
    long avail_mib  = ms->mem_avail    / 1024;
    long climit_mib = ms->commit_limit / 1024;
    long commit_mib = ms->committed    / 1024;

    mvprintw(3, 4, "MemTotal:    %6ld MiB", total_mib);
    mvprintw(4, 4, "MemAvail:    %6ld MiB", avail_mib);
    mvprintw(5, 4, "CommitLimit: %6ld MiB", climit_mib);

    if (ms->committed > ms->commit_limit)
        attron(COLOR_PAIR(CP_HIGH) | A_BOLD);
    else
        attron(COLOR_PAIR(CP_LOW));
    mvprintw(6, 4, "Committed:   %6ld MiB %s", commit_mib,
             ms->committed > ms->commit_limit ? "⚠ OVER LIMIT" : "OK");
    attroff(COLOR_PAIR(CP_HIGH) | A_BOLD);
    attroff(COLOR_PAIR(CP_LOW));

    if (ms->swap_total > 0) {
        long swap_used = (ms->swap_total - ms->swap_free) / 1024;
        long swap_tot  = ms->swap_total / 1024;
        mvprintw(7, 4, "Swap used:   %6ld / %ld MiB", swap_used, swap_tot);
    } else {
        mvprintw(7, 4, "Swap:         none configured");
    }

    /* PSI section */
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(2, 38, "MEMORY PRESSURE (PSI)");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    if (ps->available) {
        int bar_w = 28;
        mvprintw(3, 40, "some avg10: %5.2f%%", ps->some_avg10);
        draw_bar(3, 58, ps->some_avg10, bar_w,
                 ps->some_avg10 > 10.0 ? CP_BAR_HI : CP_BAR_LO);

        mvprintw(5, 40, "full avg10: %5.2f%%", ps->full_avg10);
        draw_bar(5, 58, ps->full_avg10, bar_w,
                 ps->full_avg10 > 5.0  ? CP_BAR_HI : CP_BAR_LO);

        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(7, 40, "some: ≥1 task stalled   full: all stalled");
        attroff(COLOR_PAIR(CP_NORMAL));
    } else {
        attron(COLOR_PAIR(CP_MED));
        mvprintw(3, 40, "PSI unavailable (need kernel 4.20+ + CONFIG_PSI)");
        attroff(COLOR_PAIR(CP_MED));
    }

    /* Separator */
    mvhline(9, 0, ACS_HLINE, cols);

    /* Process table header */
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(10, 2, "  %-6s  %-24s  %-10s  %-8s  %-10s",
             "PID", "PROCESS", "OOM_SCORE", "ADJ", "RSS(MiB)");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvhline(11, 0, ACS_HLINE, cols);

    int max_show = rows - 14;
    int show = (proc_count < max_show) ? proc_count : max_show;

    for (int i = 0; i < show; i++) {
        const Proc *p = &procs[i];
        int pair = p->score > 400 ? CP_HIGH :
                   p->score > 200 ? CP_MED  : CP_LOW;

        /* Immune marker */
        const char *immune = (p->adj == -1000) ? " ✓" : "  ";

        attron(COLOR_PAIR(pair));
        mvprintw(12 + i, 2,
                 "  %-6d  %-24.24s  %-10d  %-6d%s  %-10ld",
                 p->pid, p->name, p->score, p->adj, immune, p->rss_mib);
        attroff(COLOR_PAIR(pair));
    }

    /* Footer legend */
    mvhline(rows - 3, 0, ACS_HLINE, cols);
    attron(COLOR_PAIR(CP_LOW)  | A_BOLD); mvprintw(rows-2, 2, "●"); attroff(COLOR_PAIR(CP_LOW)  | A_BOLD);
    mvprintw(rows-2, 4, " low (<200)  ");
    attron(COLOR_PAIR(CP_MED)  | A_BOLD); mvprintw(rows-2,18, "●"); attroff(COLOR_PAIR(CP_MED)  | A_BOLD);
    mvprintw(rows-2,20, " medium (200-400)  ");
    attron(COLOR_PAIR(CP_HIGH) | A_BOLD); mvprintw(rows-2,39, "●"); attroff(COLOR_PAIR(CP_HIGH) | A_BOLD);
    mvprintw(rows-2,41, " high (>400)   ✓ = adj=-1000 immune");

    refresh();
}

/* ── snapshot mode (non-TTY) ─────────────────────────────── */

static void snapshot_mode(const MemStats *ms, const PsiStats *ps)
{
    printf("\033[1;34m");
    printf("┌───────────────────────────────────────────────────────────┐\n");
    printf("│            OOM Score Monitor — Snapshot Mode              │\n");
    printf("└───────────────────────────────────────────────────────────┘\033[0m\n\n");

    printf("\033[1mSystem Memory:\033[0m\n");
    printf("  MemTotal:    %6ld MiB   CommitLimit: %6ld MiB\n",
           ms->mem_total/1024, ms->commit_limit/1024);
    printf("  MemAvail:    %6ld MiB   Committed:   %6ld MiB %s\n",
           ms->mem_avail/1024, ms->committed/1024,
           ms->committed > ms->commit_limit ? "\033[1;31m⚠ OVER LIMIT\033[0m" : "");

    if (ps->available) {
        printf("\n\033[1mPSI Memory Pressure:\033[0m\n");
        printf("  some avg10: %.2f%%   full avg10: %.2f%%\n",
               ps->some_avg10, ps->full_avg10);
    }

    printf("\n\033[1mTop Processes by OOM Score:\033[0m\n");
    printf("  %-6s  %-24s  %-10s  %-8s  %-10s\n",
           "PID", "PROCESS", "OOM_SCORE", "ADJ", "RSS(MiB)");
    printf("  %-6s  %-24s  %-10s  %-8s  %-10s\n",
           "──────", "────────────────────────",
           "─────────", "───────", "──────────");

    int show = (proc_count < 20) ? proc_count : 20;
    for (int i = 0; i < show; i++) {
        const Proc *p = &procs[i];
        const char *color = p->score > 400 ? "\033[1;31m" :
                            p->score > 200 ? "\033[1;33m" : "\033[1;32m";
        printf("  %-6d  %-24.24s  %s%-10d\033[0m  %-8d  %-10ld\n",
               p->pid, p->name, color, p->score, p->adj, p->rss_mib);
    }
    printf("\n");
}

/* ── main ────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    int snapshot = (argc > 1 && strcmp(argv[1], "--snapshot") == 0);

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    if (snapshot) {
        scan_procs();
        qsort(procs, (size_t)proc_count, sizeof(Proc), cmp_score);
        MemStats ms; PsiStats ps;
        read_memstats(&ms);
        read_psi(&ps);
        snapshot_mode(&ms, &ps);
        return 0;
    }

    /* ncurses interactive mode */
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Terminal does not support colors\n");
        return 1;
    }
    start_color();
    use_default_colors();

    init_pair(CP_TITLE,  COLOR_WHITE,  COLOR_BLUE);
    init_pair(CP_HEADER, COLOR_CYAN,   -1);
    init_pair(CP_HIGH,   COLOR_RED,    -1);
    init_pair(CP_MED,    COLOR_YELLOW, -1);
    init_pair(CP_LOW,    COLOR_GREEN,  -1);
    init_pair(CP_NORMAL, COLOR_WHITE,  -1);
    init_pair(CP_ALERT,  COLOR_RED,    -1);
    init_pair(CP_BAR_HI, COLOR_RED,    COLOR_RED);
    init_pair(CP_BAR_LO, COLOR_GREEN,  COLOR_GREEN);

    while (running) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        scan_procs();
        qsort(procs, (size_t)proc_count, sizeof(Proc), cmp_score);

        MemStats ms;
        PsiStats ps;
        read_memstats(&ms);
        read_psi(&ps);
        draw_screen(&ms, &ps);

        /* Sleep 1s in 50ms slices for responsive quit */
        for (int i = 0; i < 20 && running; i++) {
            usleep(50000);
            ch = getch();
            if (ch == 'q' || ch == 'Q') { running = 0; break; }
        }
    }

    endwin();
    return 0;
}
