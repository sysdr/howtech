#define _GNU_SOURCE

#include <curses.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;

static long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void on_sigint(int signo)
{
    (void)signo;
    g_stop = 1;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--text] [--samples N] [--interval-ms MS]\n"
            "  --text         print metric snapshots to stdout\n"
            "  --samples N    number of snapshots in --text mode (default 8)\n"
            "  --interval-ms  interval between snapshots (default 250)\n",
            argv0);
}

static unsigned long long read_ull_file(const char *path, int *ok)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        *ok = 0;
        return 0;
    }
    unsigned long long v = 0;
    if (fscanf(f, "%llu", &v) != 1) {
        fclose(f);
        *ok = 0;
        return 0;
    }
    fclose(f);
    *ok = 1;
    return v;
}

static unsigned long long read_ctxt(int *ok)
{
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        *ok = 0;
        return 0;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "ctxt ", 5) == 0) {
            unsigned long long v = 0;
            if (sscanf(line + 5, "%llu", &v) == 1) {
                fclose(f);
                *ok = 1;
                return v;
            }
        }
    }
    fclose(f);
    *ok = 0;
    return 0;
}

static int have_path(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static void draw_header(int row, const char *title)
{
    attron(A_BOLD);
    mvprintw(row, 2, "%s", title);
    attroff(A_BOLD);
}

static int run_curses(void)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    long long last_tick = now_ns();
    unsigned long long ctxt_prev = 0;
    int ctxt_prev_ok = 0;
    long long ctxt_prev_ts = 0;

    int reset_requested = 1;
    int cycles = 0;

    while (!g_stop) {
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == ' ') {
            reset_requested = 1;
        }

        long long t = now_ns();
        if (reset_requested || t - last_tick > 250 * 1000 * 1000LL) {
            last_tick = t;
            cycles++;

            erase();
            attron(A_REVERSE);
            mvprintw(0, 0,
                     " sched_ext revert monitor  (q quit, space reset) ");
            attroff(A_REVERSE);

            draw_header(2, "Kernel / sched_ext status");
            mvprintw(3, 4, "kernel: %s", have_path("/sys/kernel/sched_ext")
                                            ? "sched_ext present"
                                            : "sched_ext not present");
            if (have_path("/sys/kernel/sched_ext/state")) {
                int ok = 0;
                unsigned long long state =
                    read_ull_file("/sys/kernel/sched_ext/state", &ok);
                if (ok)
                    mvprintw(4, 4, "scx state: %llu", state);
            } else {
                mvprintw(4, 4, "scx state: (unavailable)");
            }

            draw_header(6, "Core metrics");
            int ctxt_ok = 0;
            unsigned long long ctxt = read_ctxt(&ctxt_ok);
            if (ctxt_ok) {
                double rate = 0.0;
                if (ctxt_prev_ok) {
                    double dt =
                        (double)(t - ctxt_prev_ts) / (double)1000000000.0;
                    if (dt > 0.0 && ctxt >= ctxt_prev)
                        rate = (double)(ctxt - ctxt_prev) / dt;
                }
                mvprintw(7, 4, "context switches: %llu  (%.0f / sec)", ctxt,
                         rate);
                ctxt_prev = ctxt;
                ctxt_prev_ok = 1;
                ctxt_prev_ts = t;
            } else {
                mvprintw(7, 4, "context switches: (unavailable)");
            }

            int ftrace = have_path("/sys/kernel/debug/tracing/trace");
            mvprintw(8, 4, "ftrace: %s",
                     ftrace ? "/sys/kernel/debug/tracing available"
                            : "not available");

            draw_header(10, "Instructions / demo pointers");
            mvprintw(11, 4, "This monitor is the 'dashboard'. Metrics should");
            mvprintw(12, 4, "update every ~250ms. Press space to reset cycle.");
            mvprintw(13, 4, "If sched_ext exists, you can watch dmesg for");
            mvprintw(14, 4, "revert messages while running a real scx scheduler.");

            mvprintw(LINES - 2, 2, "cycles: %d", cycles);
            refresh();

            if (reset_requested) {
                reset_requested = 0;
                ctxt_prev_ok = 0;
                ctxt_prev = 0;
                ctxt_prev_ts = 0;
            }
        }

        usleep(25 * 1000);
    }

    endwin();
    return 0;
}

static void sleep_ms(int ms)
{
    if (ms < 0)
        ms = 0;
    usleep((useconds_t)ms * 1000U);
}

static int run_text(int samples, int interval_ms)
{
    unsigned long long last_ctxt = 0;
    int last_ok = 0;
    long long last_ts = 0;

    for (int i = 0; i < samples && !g_stop; i++) {
        long long t = now_ns();
        int ctxt_ok = 0;
        unsigned long long ctxt = read_ctxt(&ctxt_ok);
        double rate = 0.0;

        if (ctxt_ok && last_ok) {
            double dt = (double)(t - last_ts) / 1e9;
            if (dt > 0.0 && ctxt >= last_ctxt)
                rate = (double)(ctxt - last_ctxt) / dt;
        }

        int scx_present = have_path("/sys/kernel/sched_ext");
        fprintf(stdout,
                "sample=%d scx_present=%d ctxt=%s rate_per_s=%.0f ftrace=%d\n",
                i + 1, scx_present, ctxt_ok ? "ok" : "na", rate,
                have_path("/sys/kernel/debug/tracing/trace"));

        if (ctxt_ok) {
            fprintf(stdout, "  ctxt_value=%llu\n", ctxt);
            last_ctxt = ctxt;
            last_ok = 1;
            last_ts = t;
        } else {
            last_ok = 0;
            last_ctxt = 0;
            last_ts = 0;
        }
        fflush(stdout);
        sleep_ms(interval_ms);
    }

    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    int text = 0;
    int samples = 8;
    int interval_ms = 250;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--text") == 0) {
            text = 1;
        } else if (strcmp(argv[i], "--samples") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            samples = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--interval-ms") == 0) {
            if (i + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            interval_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    if (text)
        return run_text(samples, interval_ms);

    return run_curses();
}
