/*
 * monitor.c — Real-time verification results monitor (ncurses)
 * Displays KASAN / lockdep / KCSAN / BPF verifier results as they run
 */
#define _GNU_SOURCE
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef struct {
    const char *name;
    const char *binary;
    int         pid;
    int         exit_code;
    int         done;
    double      elapsed_ms;
    struct timespec start;
} check_t;

static check_t checks[] = {
    { "KASAN (use-after-free / OOB / double-free)", "./kasan_sim",   0, 0, 0, 0.0, {0} },
    { "lockdep (AB-BA deadlock detection)",          "./lockdep_sim", 0, 0, 0, 0.0, {0} },
    { "BPF Verifier (abstract interpretation)",      "./bpf_verify_sim", 0, 0, 0, 0.0, {0} },
    { "KCSAN (data race detection)",                 "./kcsan_sim",   0, 0, 0, 0.0, {0} },
};
#define N_CHECKS (int)(sizeof(checks)/sizeof(checks[0]))

static double elapsed_ms(struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start->tv_sec) * 1000.0 +
           (now.tv_nsec - start->tv_nsec) / 1e6;
}

static void draw_header(int row)
{
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(row, 2, "┌─────────────────────────────────────────────────────────────────────┐");
    mvprintw(row+1, 2, "│        KERNEL VERIFICATION SUITE — Systems Programming Deep Dive      │");
    mvprintw(row+2, 2, "└─────────────────────────────────────────────────────────────────────┘");
    attroff(A_BOLD | COLOR_PAIR(4));
}

static void draw_check(int row, int idx, double now_ms)
{
    check_t *c = &checks[idx];
    mvprintw(row, 2, "  %s", c->name);

    if (!c->done) {
        double ms = c->pid ? elapsed_ms(&c->start) : 0;
        attron(COLOR_PAIR(3));
        mvprintw(row+1, 6, "[ RUNNING  %.0fms ]", ms);
        attroff(COLOR_PAIR(3));
    } else if (c->exit_code == 0) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(row+1, 6, "[ PASS  %.0fms ]    ", c->elapsed_ms);
        attroff(COLOR_PAIR(2) | A_BOLD);
    } else {
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(row+1, 6, "[ FAIL  exit=%d ]  ", c->exit_code);
        attroff(COLOR_PAIR(1) | A_BOLD);
    }
    (void)now_ms;
}

static void draw_footer(int row, int done, int passed)
{
    attron(COLOR_PAIR(4));
    mvprintw(row, 2, "──────────────────────────────────────────────────────────────────────");
    attroff(COLOR_PAIR(4));
    if (done == N_CHECKS) {
        if (passed == N_CHECKS) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(row+1, 4, "✓  All %d verification checks passed                ", N_CHECKS);
        } else {
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(row+1, 4, "✗  %d/%d checks passed — see output above           ", passed, N_CHECKS);
        }
        attroff(A_BOLD | COLOR_PAIR(2) | COLOR_PAIR(1));
        mvprintw(row+2, 4, "Press any key to exit...");
    } else {
        mvprintw(row+1, 4, "Checks complete: %d/%d | Press Ctrl+C to abort", done, N_CHECKS);
    }
}

int main(void)
{
    initscr();
    start_color();
    noecho();
    cbreak();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    init_pair(1, COLOR_RED,    COLOR_BLACK);
    init_pair(2, COLOR_GREEN,  COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_CYAN,   COLOR_BLACK);
    init_pair(5, COLOR_WHITE,  COLOR_BLACK);

    /* Start all checks in parallel */
    for (int i = 0; i < N_CHECKS; i++) {
        clock_gettime(CLOCK_MONOTONIC, &checks[i].start);
        checks[i].pid = fork();
        if (checks[i].pid == 0) {
            /* child: redirect stdout/stderr to /dev/null for monitor clarity */
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
            execlp(checks[i].binary, checks[i].binary, NULL);
            _exit(127);
        }
    }

    int done = 0, passed = 0;
    while (1) {
        /* Check for completions */
        for (int i = 0; i < N_CHECKS; i++) {
            if (!checks[i].done && checks[i].pid > 0) {
                int status;
                pid_t r = waitpid(checks[i].pid, &status, WNOHANG);
                if (r == checks[i].pid) {
                    checks[i].done = 1;
                    checks[i].exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                    checks[i].elapsed_ms = elapsed_ms(&checks[i].start);
                    done++;
                    if (checks[i].exit_code == 0) passed++;
                }
            }
        }

        double now_ms = 0; /* for animation */
        clear();
        draw_header(1);

        for (int i = 0; i < N_CHECKS; i++)
            draw_check(5 + i * 3, i, now_ms);

        draw_footer(5 + N_CHECKS * 3 + 1, done, passed);
        refresh();

        if (done == N_CHECKS) {
            nodelay(stdscr, FALSE);
            getch();
            break;
        }

        int ch = getch();
        if (ch == 3 || ch == 'q') break; /* Ctrl+C or q */

        struct timespec ts = { .tv_nsec = 80000000 }; /* 80ms refresh */
        nanosleep(&ts, NULL);
    }

    endwin();
    printf("\nVerification complete: %d/%d passed\n", passed, N_CHECKS);
    return (passed == N_CHECKS) ? 0 : 1;
}
