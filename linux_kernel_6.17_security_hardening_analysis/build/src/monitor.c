/*
 * monitor.c — ncurses real-time kernel security feature monitor
 *
 * Displays live security feature status, ASLR entropy, and seccomp
 * statistics. Refreshes every second.
 *
 * Build: gcc -Wall -Wextra -Werror -O2 -o monitor monitor.c -lncurses
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

static volatile sig_atomic_t running = 1;
static void sig_handler(int s) { (void)s; running = 0; }

#define C_TITLE    1
#define C_GOOD     2
#define C_WARN     3
#define C_BAD      4
#define C_DIM      5
#define C_HEADER   6
#define C_VALUE    7

static int read_sysctl_int(const char *path, int def)
{
    char buf[64] = {0};
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return def;
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (n <= 0) return def;
    return atoi(buf);
}

static void read_sysctl_str(const char *path, char *out, size_t len)
{
    memset(out, 0, len);
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) { snprintf(out, len, "N/A"); return; }
    ssize_t n = read(fd, out, len-1);
    close(fd);
    if (n > 0) {
        out[n] = '\0';
        char *nl = strchr(out, '\n');
        if (nl) *nl = '\0';
    }
}

static void draw_bar(WINDOW *win, int row, int col, int width,
                     double pct, int color_pair)
{
    int filled = (int)(pct * width / 100.0);
    if (filled > width) filled = width;
    wattron(win, COLOR_PAIR(color_pair));
    for (int i = 0; i < width; i++)
        mvwaddch(win, row, col + i, (i < filled) ? ACS_BLOCK : ' ');
    wattroff(win, COLOR_PAIR(color_pair));
}

static void draw_feature_row(WINDOW *win, int row,
                              const char *label, int val,
                              int good_val, const char *desc)
{
    mvwprintw(win, row, 2, "%-28s", label);
    wattron(win, COLOR_PAIR(C_VALUE) | A_BOLD);
    mvwprintw(win, row, 30, "%2d", val);
    wattroff(win, COLOR_PAIR(C_VALUE) | A_BOLD);

    mvwaddch(win, row, 33, ' ');
    if (val == good_val) {
        wattron(win, COLOR_PAIR(C_GOOD));
        mvwprintw(win, row, 34, "✓ %-38s", desc);
        wattroff(win, COLOR_PAIR(C_GOOD));
    } else {
        wattron(win, COLOR_PAIR(C_WARN));
        mvwprintw(win, row, 34, "△ %-38s", desc);
        wattroff(win, COLOR_PAIR(C_WARN));
    }
}

int main(void)
{
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(1000);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(C_TITLE,  COLOR_CYAN,    -1);
        init_pair(C_GOOD,   COLOR_GREEN,   -1);
        init_pair(C_WARN,   COLOR_YELLOW,  -1);
        init_pair(C_BAD,    COLOR_RED,     -1);
        init_pair(C_DIM,    COLOR_WHITE,   -1);
        init_pair(C_HEADER, COLOR_BLUE,    -1);
        init_pair(C_VALUE,  COLOR_CYAN,    -1);
    }

    struct utsname uts;
    uname(&uts);

    while (running) {
        clear();
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        (void)rows;

        /* ── Title bar ──────────────────────────────────────────────── */
        wattron(stdscr, COLOR_PAIR(C_TITLE) | A_BOLD);
        mvprintw(0, 0, " %-*s", cols - 1, " Linux Kernel Security Monitor · 6.17 Hardening");
        wattroff(stdscr, COLOR_PAIR(C_TITLE) | A_BOLD);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

        mvprintw(1, 2, "Kernel: ");
        wattron(stdscr, COLOR_PAIR(C_VALUE));
        printw("%s %s", uts.sysname, uts.release);
        wattroff(stdscr, COLOR_PAIR(C_VALUE));
        mvprintw(1, cols - 12, "[%s]", timebuf);

        /* ── Section: Kernel Pointer & Info Leak Mitigations ───────── */
        wattron(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);
        mvprintw(3, 2, "── Kernel Pointer & Info Leak Mitigations ──");
        wattroff(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);

        int kptr     = read_sysctl_int("/proc/sys/kernel/kptr_restrict",        0);
        int dmesg    = read_sysctl_int("/proc/sys/kernel/dmesg_restrict",       0);
        int aslr     = read_sysctl_int("/proc/sys/kernel/randomize_va_space",   0);
        int mmap_rnd = read_sysctl_int("/proc/sys/vm/mmap_rnd_bits",            28);

        draw_feature_row(stdscr, 4,  "kptr_restrict",        kptr,  2, "Kernel ptrs hidden (KASLR protected)");
        draw_feature_row(stdscr, 5,  "dmesg_restrict",       dmesg, 1, "Ring buffer hidden from non-root");
        draw_feature_row(stdscr, 6,  "randomize_va_space",   aslr,  2, "Full ASLR: stack+mmap+brk+VDSO");

        mvprintw(7, 2, "%-28s", "mmap_rnd_bits (entropy)");
        wattron(stdscr, COLOR_PAIR(C_VALUE) | A_BOLD);
        mvprintw(7, 30, "%2d", mmap_rnd);
        wattroff(stdscr, COLOR_PAIR(C_VALUE) | A_BOLD);
        draw_bar(stdscr, 7, 34, 20, (double)mmap_rnd / 56.0 * 100.0,
                 mmap_rnd >= 32 ? C_GOOD : C_WARN);
        mvprintw(7, 55, " %d bits = 2^%d bases", mmap_rnd, mmap_rnd);

        /* ── Section: Syscall & BPF Restrictions ───────────────────── */
        wattron(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);
        mvprintw(9, 2, "── Syscall & BPF Restrictions ──");
        wattroff(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);

        int bpf_unpriv = read_sysctl_int("/proc/sys/kernel/unprivileged_bpf_disabled", -1);
        int perf_para  = read_sysctl_int("/proc/sys/kernel/perf_event_paranoid",       -1);
        int io_uring_d = read_sysctl_int("/proc/sys/kernel/io_uring_disabled",          0);
        int yama       = read_sysctl_int("/proc/sys/kernel/yama/ptrace_scope",         -1);

        draw_feature_row(stdscr, 10, "unprivileged_bpf_disabled", bpf_unpriv, 1,
                         "Unprivileged BPF blocked");
        draw_feature_row(stdscr, 11, "perf_event_paranoid",       perf_para,  3,
                         "perf blocked (KASLR brute-force mitigation)");
        draw_feature_row(stdscr, 12, "io_uring_disabled",         io_uring_d, 0,
                         "0=enabled, 1=restricted, 2=full disable");
        draw_feature_row(stdscr, 13, "yama/ptrace_scope",         yama,       1,
                         "YAMA: only parent can ptrace child");

        /* ── Section: Memory Hardening ─────────────────────────────── */
        wattron(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);
        mvprintw(15, 2, "── Memory & Stack Hardening ──");
        wattroff(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);

        char slub_debug[64], page_poison[64];
        read_sysctl_str("/sys/kernel/slab/kmalloc-64/reclaim_account", slub_debug, sizeof(slub_debug));
        read_sysctl_str("/proc/sys/kernel/panic_on_oops", page_poison, sizeof(page_poison));

        int panic_oops = atoi(page_poison);
        draw_feature_row(stdscr, 16, "panic_on_oops", panic_oops, 1,
                         "Crash on kernel oops (no partial exploitation)");

        /* Sysrq */
        int sysrq = read_sysctl_int("/proc/sys/kernel/sysrq", 1);
        draw_feature_row(stdscr, 17, "kernel.sysrq",  sysrq, 0,
                         "0=disabled (hardened), 1=enabled");

        /* ── Section: Live Entropy Pool ─────────────────────────────── */
        wattron(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);
        mvprintw(19, 2, "── Random Entropy Pool ──");
        wattroff(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);

        int entropy_avail = read_sysctl_int("/proc/sys/kernel/random/entropy_avail", 0);
        int entropy_pool  = read_sysctl_int("/proc/sys/kernel/random/poolsize",      4096);

        mvprintw(20, 2, "%-28s", "entropy_avail");
        wattron(stdscr, COLOR_PAIR(C_VALUE) | A_BOLD);
        mvprintw(20, 30, "%4d", entropy_avail);
        wattroff(stdscr, COLOR_PAIR(C_VALUE) | A_BOLD);
        double entropy_pct = entropy_pool > 0 ?
            (double)entropy_avail / (double)entropy_pool * 100.0 : 0.0;
        if (entropy_pct > 100.0) entropy_pct = 100.0;
        draw_bar(stdscr, 20, 35, 30, entropy_pct,
                 entropy_avail >= 256 ? C_GOOD : C_WARN);
        mvprintw(20, 66, " / %d bits", entropy_pool);

        /* ── Legend ─────────────────────────────────────────────────── */
        wattron(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);
        mvprintw(22, 2, "── Legend ──");
        wattroff(stdscr, COLOR_PAIR(C_HEADER) | A_BOLD);

        wattron(stdscr, COLOR_PAIR(C_GOOD));
        mvprintw(23, 2, "✓ Hardened");
        wattroff(stdscr, COLOR_PAIR(C_GOOD));
        wattron(stdscr, COLOR_PAIR(C_WARN));
        mvprintw(23, 15, "△ Suboptimal");
        wattroff(stdscr, COLOR_PAIR(C_WARN));
        wattron(stdscr, COLOR_PAIR(C_DIM));
        mvprintw(23, 30, "Reads /proc/sys/kernel/* · /sys/kernel/*  |  q to quit");
        wattroff(stdscr, COLOR_PAIR(C_DIM));

        refresh();

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;
    }

    endwin();
    printf("\nMonitor exited.\n");
    return EXIT_SUCCESS;
}
