/*
 * percpu_monitor.c
 * Real-time ncurses monitor showing per-thread counter accumulation
 * and live aggregation — mirrors what the kernel does when userspace
 * calls read() on /proc/net/dev or similar per-CPU stats files.
 *
 * Build: gcc -Wall -Wextra -Werror -O2 -pthread -lncurses -o percpu_monitor percpu_monitor.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <ncurses.h>

#define MAX_THREADS   8
#define CACHE_LINE    64
#define TARGET_OPS    50000000ULL   /* 50M total ops */
#define REFRESH_MS    100           /* monitor refresh interval */

/* ── Per-CPU counter (cache-line aligned, no false sharing) ── */
typedef struct {
    atomic_long value;
    atomic_long rate;           /* ops/sec snapshot */
    char _pad[CACHE_LINE - 2 * sizeof(atomic_long)];
} __attribute__((aligned(CACHE_LINE))) cpu_slot_t;

static cpu_slot_t slots[MAX_THREADS];
static atomic_int g_running;
static atomic_long g_global_atomic;    /* the "bad" counter for comparison */
static int g_nthreads;

/* ── Thread state ────────────────────────────────────────── */
typedef struct {
    int    tid;
    double elapsed_ms;
} worker_arg_t;

static worker_arg_t wargs[MAX_THREADS];

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e3 + (double)ts.tv_nsec / 1e6;
}

/* ── Worker: increments per-CPU slot AND the shared global ── */
static void *worker(void *arg)
{
    worker_arg_t *a = (worker_arg_t *)arg;
    long local_count = 0;
    double last_snapshot = now_ms();
    long   last_val = 0;

    while (atomic_load_explicit(&g_running, memory_order_relaxed)) {
        /* Burst of work before polling g_running */
        for (int burst = 0; burst < 10000; burst++) {
            atomic_fetch_add_explicit(&slots[a->tid].value, 1, memory_order_relaxed);
            atomic_fetch_add_explicit(&g_global_atomic, 1, memory_order_relaxed);
            local_count++;
        }

        /* Compute rate every ~200ms */
        double now = now_ms();
        if (now - last_snapshot >= 200.0) {
            long cur = atomic_load(&slots[a->tid].value);
            long rate = (long)((double)(cur - last_val) / ((now - last_snapshot) / 1000.0));
            atomic_store(&slots[a->tid].rate, rate);
            last_val = cur;
            last_snapshot = now;
        }
    }
    a->elapsed_ms = now_ms();
    (void)local_count;
    return NULL;
}

/* ── Draw a horizontal bar ──────────────────────────────── */
static void draw_bar(WINDOW *w, int y, int x, int width, double fraction, int color_pair)
{
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    int filled = (int)(fraction * width);

    wattron(w, COLOR_PAIR(color_pair));
    for (int i = 0; i < filled; i++)
        mvwaddch(w, y, x + i, ACS_BLOCK);
    wattroff(w, COLOR_PAIR(color_pair));

    /* empty portion */
    wattron(w, COLOR_PAIR(7));
    for (int i = filled; i < width; i++)
        mvwaddch(w, y, x + i, ACS_CKBOARD);
    wattroff(w, COLOR_PAIR(7));
}

/* ── Format large numbers ───────────────────────────────── */
static void fmt_num(long n, char *buf, size_t sz)
{
    if (n >= 1000000000L)      snprintf(buf, sz, "%.2fB", n / 1e9);
    else if (n >= 1000000L)    snprintf(buf, sz, "%.2fM", n / 1e6);
    else if (n >= 1000L)       snprintf(buf, sz, "%.1fK", n / 1e3);
    else                       snprintf(buf, sz, "%ld", n);
}

/* ── Monitor main loop ──────────────────────────────────── */
static void run_monitor(void)
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    start_color();

    /* Colour pairs */
    init_pair(1, COLOR_GREEN,   COLOR_BLACK);
    init_pair(2, COLOR_CYAN,    COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_RED,     COLOR_BLACK);
    init_pair(5, COLOR_WHITE,   COLOR_BLACK);
    init_pair(6, COLOR_BLUE,    COLOR_BLACK);
    init_pair(7, COLOR_BLACK,   COLOR_BLACK);
    init_pair(8, COLOR_MAGENTA, COLOR_BLACK);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW *w_title  = newwin(3,  cols,          0, 0);
    WINDOW *w_slots  = newwin(rows - 14, cols,   3, 0);
    WINDOW *w_agg    = newwin(5,  cols, rows - 11, 0);
    WINDOW *w_mesi   = newwin(6,  cols, rows - 6,  0);

    double t_start = now_ms();

    while (atomic_load(&g_running)) {
        /* Check for 'q' key */
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            atomic_store(&g_running, 0);
            break;
        }

        double now = now_ms();
        double elapsed_s = (now - t_start) / 1000.0;

        /* Aggregate */
        long total_percpu = 0;
        long total_rate   = 0;
        for (int i = 0; i < g_nthreads; i++) {
            total_percpu += atomic_load(&slots[i].value);
            total_rate   += atomic_load(&slots[i].rate);
        }
        long global_val = atomic_load(&g_global_atomic);
        long max_per_slot = 0;
        for (int i = 0; i < g_nthreads; i++) {
            long v = atomic_load(&slots[i].value);
            if (v > max_per_slot) max_per_slot = v;
        }

        /* ─── Title ─── */
        werase(w_title);
        wattron(w_title, COLOR_PAIR(2) | A_BOLD);
        box(w_title, 0, 0);
        mvwprintw(w_title, 1, 2, "  per-CPU Counter Monitor  |  "
                  "threads: %d  |  elapsed: %.1fs  |  press 'q' to stop",
                  g_nthreads, elapsed_s);
        wattroff(w_title, COLOR_PAIR(2) | A_BOLD);
        wrefresh(w_title);

        /* ─── Per-slot view ─── */
        werase(w_slots);
        box(w_slots, 0, 0);
        wattron(w_slots, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(w_slots, 0, 2, " Per-CPU Slots (cache-line isolated) ");
        wattroff(w_slots, COLOR_PAIR(5) | A_BOLD);

        int bar_w = cols - 30;
        if (bar_w < 10) bar_w = 10;

        for (int i = 0; i < g_nthreads; i++) {
            long val  = atomic_load(&slots[i].value);
            long rate = atomic_load(&slots[i].rate);
            double frac = (max_per_slot > 0) ? (double)val / max_per_slot : 0.0;
            char vbuf[24], rbuf[24];
            fmt_num(val,  vbuf, sizeof(vbuf));
            fmt_num(rate, rbuf, sizeof(rbuf));

            int y = 1 + i * 2;
            int col_pair = 1 + (i % 5);

            wattron(w_slots, COLOR_PAIR(col_pair));
            mvwprintw(w_slots, y, 2, "CPU%-2d │", i);
            wattroff(w_slots, COLOR_PAIR(col_pair));

            draw_bar(w_slots, y, 9, bar_w, frac, col_pair);
            mvwprintw(w_slots, y, 9 + bar_w + 1, "%-7s  %-8s ops/s", vbuf, rbuf);
        }
        wrefresh(w_slots);

        /* ─── Aggregation ─── */
        werase(w_agg);
        box(w_agg, 0, 0);
        wattron(w_agg, COLOR_PAIR(3) | A_BOLD);
        mvwprintw(w_agg, 0, 2, " Aggregation  (kernel: sum all per_cpu_ptr values) ");
        wattroff(w_agg, COLOR_PAIR(3) | A_BOLD);

        char tbuf[24], gbuf[24], rtbuf[24];
        fmt_num(total_percpu, tbuf, sizeof(tbuf));
        fmt_num(global_val,   gbuf, sizeof(gbuf));
        fmt_num(total_rate,  rtbuf, sizeof(rtbuf));

        wattron(w_agg, COLOR_PAIR(1));
        mvwprintw(w_agg, 1, 2, "  per-CPU sum  : %s ops   (approximate — racy reads)", tbuf);
        wattroff(w_agg, COLOR_PAIR(1));
        wattron(w_agg, COLOR_PAIR(4));
        mvwprintw(w_agg, 2, 2, "  global atomic: %s ops   (exact — every inc paid cache miss)", gbuf);
        wattroff(w_agg, COLOR_PAIR(4));
        wattron(w_agg, COLOR_PAIR(3));
        mvwprintw(w_agg, 3, 2, "  aggregate rate: %s ops/s across %d CPUs", rtbuf, g_nthreads);
        wattroff(w_agg, COLOR_PAIR(3));
        wrefresh(w_agg);

        /* ─── MESI legend ─── */
        werase(w_mesi);
        box(w_mesi, 0, 0);
        wattron(w_mesi, COLOR_PAIR(2) | A_BOLD);
        mvwprintw(w_mesi, 0, 2, " MESI Cache State ");
        wattroff(w_mesi, COLOR_PAIR(2) | A_BOLD);
        wattron(w_mesi, COLOR_PAIR(1));
        mvwprintw(w_mesi, 1, 2, "  [E] per-CPU slots  → Exclusive: each CPU owns its cache line, zero cross-CPU invalidations");
        wattroff(w_mesi, COLOR_PAIR(1));
        wattron(w_mesi, COLOR_PAIR(4));
        mvwprintw(w_mesi, 2, 2, "  [M/I] global atomic → Modified on writer, Invalid on all others → bus broadcast every write");
        wattroff(w_mesi, COLOR_PAIR(4));
        wattron(w_mesi, COLOR_PAIR(3));
        mvwprintw(w_mesi, 3, 2, "  Kernel: this_cpu_inc(var) → single 'add %%gs:offset,1' (x86-64) — no lock prefix needed");
        wattroff(w_mesi, COLOR_PAIR(3));
        wattron(w_mesi, COLOR_PAIR(7));
        mvwprintw(w_mesi, 4, 2, "  Press 'q' to exit monitor and run full benchmark");
        wattroff(w_mesi, COLOR_PAIR(7));
        wrefresh(w_mesi);

        struct timespec ts = { .tv_sec = 0, .tv_nsec = REFRESH_MS * 1000000L };
        nanosleep(&ts, NULL);
    }

    delwin(w_mesi);
    delwin(w_agg);
    delwin(w_slots);
    delwin(w_title);
    endwin();
}

int main(void)
{
    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus <= 0) ncpus = 2;
    g_nthreads = (ncpus > MAX_THREADS) ? MAX_THREADS : (int)ncpus;

    memset(slots, 0, sizeof(slots));
    atomic_store(&g_global_atomic, 0);
    atomic_store(&g_running, 1);

    pthread_t tids[MAX_THREADS];
    for (int i = 0; i < g_nthreads; i++) {
        wargs[i].tid = i;
        if (pthread_create(&tids[i], NULL, worker, &wargs[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    run_monitor();

    atomic_store(&g_running, 0);
    for (int i = 0; i < g_nthreads; i++)
        pthread_join(tids[i], NULL);

    return 0;
}
