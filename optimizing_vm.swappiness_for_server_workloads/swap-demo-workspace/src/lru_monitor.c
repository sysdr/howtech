/*
 * lru_monitor.c - Real-time LRU reclaim activity monitor
 * Watches /proc/vmstat deltas to show live swap pressure
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -lncurses -o lru_monitor lru_monitor.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h>
#include <errno.h>
#include <stdint.h>

#define REFRESH_MS 1000
#define HISTORY    40

typedef struct {
    uint64_t pgscank, pgscand;
    uint64_t pswpin, pswpout;
    uint64_t pgsteal_kswapd, pgsteal_direct;
    uint64_t pgfault, pgmajfault;
} Stats;

typedef struct {
    uint64_t mem_free_kb, swap_used_kb, swap_total_kb;
    uint64_t anon_kb, mem_total_kb;
} Mem;

static volatile sig_atomic_t g_running = 1;
static void sig_handler(int s) { (void)s; g_running = 0; }

static int read_vmstats(Stats *s) {
    FILE *f = fopen("/proc/vmstat", "r");
    if (!f) return -1;
    char key[64]; uint64_t v;
    memset(s, 0, sizeof(*s));
    while (fscanf(f, "%63s %lu", key, &v) == 2) {
        if      (!strcmp(key,"pgscank"))          s->pgscank = v;
        else if (!strcmp(key,"pgscand"))          s->pgscand = v;
        else if (!strcmp(key,"pswpin"))           s->pswpin = v;
        else if (!strcmp(key,"pswpout"))          s->pswpout = v;
        else if (!strcmp(key,"pgsteal_kswapd"))   s->pgsteal_kswapd = v;
        else if (!strcmp(key,"pgsteal_direct"))   s->pgsteal_direct = v;
        else if (!strcmp(key,"pgfault"))          s->pgfault = v;
        else if (!strcmp(key,"pgmajfault"))       s->pgmajfault = v;
    }
    fclose(f); return 0;
}

static int read_mem(Mem *m) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;
    char key[64], unit[16]; uint64_t v;
    uint64_t stotal = 0, sfree = 0;
    memset(m, 0, sizeof(*m));
    while (fscanf(f, "%63s %lu %15s", key, &v, unit) >= 2) {
        if      (!strcmp(key,"MemTotal:"))   m->mem_total_kb = v;
        else if (!strcmp(key,"MemFree:"))    m->mem_free_kb  = v;
        else if (!strcmp(key,"AnonPages:"))  m->anon_kb      = v;
        else if (!strcmp(key,"SwapTotal:"))  stotal = v;
        else if (!strcmp(key,"SwapFree:"))   sfree  = v;
    }
    m->swap_total_kb = stotal;
    m->swap_used_kb  = stotal - sfree;
    fclose(f); return 0;
}

static void draw_hbar(WINDOW *w, int y, int x, int width,
                      uint64_t val, uint64_t max, int color_pair) {
    int filled = (max > 0) ? (int)((double)val / max * width) : 0;
    if (filled > width) filled = width;
    wattron(w, COLOR_PAIR(color_pair));
    for (int i = 0; i < filled; i++) mvwaddch(w, y, x + i, ACS_BLOCK);
    wattroff(w, COLOR_PAIR(color_pair));
    wattron(w, COLOR_PAIR(3));
    for (int i = filled; i < width; i++) mvwaddch(w, y, x + i, ACS_CKBOARD);
    wattroff(w, COLOR_PAIR(3));
}

static uint64_t udiff(uint64_t a, uint64_t b) {
    return (a > b) ? a - b : 0;
}

static char g_swappiness[8] = "?";
static void load_swappiness(void) {
    FILE *f = fopen("/proc/sys/vm/swappiness", "r");
    if (!f) return;
    char *ret = fgets(g_swappiness, sizeof(g_swappiness), f);
    if (ret) {
        size_t l = strlen(g_swappiness);
        if (l && g_swappiness[l-1] == '\n') g_swappiness[l-1] = '\0';
    }
    fclose(f);
}

int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    initscr();
    cbreak(); noecho(); curs_set(0);
    timeout(REFRESH_MS);

    if (has_colors()) {
        start_color(); use_default_colors();
        init_pair(1, COLOR_GREEN,  -1);  /* good / kswapd */
        init_pair(2, COLOR_RED,    -1);  /* bad / direct */
        init_pair(3, COLOR_BLACK,  -1);  /* empty bar */
        init_pair(4, COLOR_CYAN,   -1);  /* header */
        init_pair(5, COLOR_YELLOW, -1);  /* warning */
        init_pair(6, COLOR_BLUE,   -1);  /* info */
        init_pair(7, COLOR_WHITE,  -1);  /* normal */
    }

    load_swappiness();

    Stats prev, cur;
    Mem mem;
    memset(&prev, 0, sizeof(prev));
    read_vmstats(&prev);

    /* History arrays for sparkline */
    uint64_t hist_kswapd[HISTORY] = {0};
    uint64_t hist_direct[HISTORY] = {0};
    int  hist_idx = 0;
    uint64_t peak_kswapd = 1, peak_direct = 1, peak_pswpout = 1;

    int rows, cols;

    while (g_running) {
        getmaxyx(stdscr, rows, cols);
        (void)rows;
        int w = cols - 4;
        if (w < 40) w = 40;

        int ch = getch();
        if (ch == 'q' || ch == 'Q') break;

        read_vmstats(&cur);
        read_mem(&mem);

        uint64_t dk = udiff(cur.pgscank, prev.pgscank);
        uint64_t dd = udiff(cur.pgscand, prev.pgscand);
        uint64_t dpi = udiff(cur.pswpin,  prev.pswpin);
        uint64_t dpo = udiff(cur.pswpout, prev.pswpout);
        uint64_t dmf = udiff(cur.pgmajfault, prev.pgmajfault);
        uint64_t dsk = udiff(cur.pgsteal_kswapd, prev.pgsteal_kswapd);
        uint64_t dsd = udiff(cur.pgsteal_direct,  prev.pgsteal_direct);

        hist_kswapd[hist_idx]  = dk;
        hist_direct[hist_idx]  = dd;
        hist_idx = (hist_idx + 1) % HISTORY;

        if (dk  > peak_kswapd)  peak_kswapd  = dk;
        if (dd  > peak_direct)  peak_direct  = dd;
        if (dpo > peak_pswpout) peak_pswpout = dpo;
        (void)peak_pswpout;

        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm_info);

        clear();
        int y = 0;

        /* Header */
        wattron(stdscr, COLOR_PAIR(4) | A_BOLD);
        mvprintw(y++, 0, "  LRU Reclaim Monitor │ vm.swappiness=%-3s │ %s │ Press q to quit",
                 g_swappiness, timebuf);
        wattroff(stdscr, COLOR_PAIR(4) | A_BOLD);

        wattron(stdscr, COLOR_PAIR(3));
        mvhline(y++, 0, ACS_HLINE, cols);
        wattroff(stdscr, COLOR_PAIR(3));

        /* Memory section */
        wattron(stdscr, A_BOLD);
        mvprintw(y++, 2, "MEMORY");
        wattroff(stdscr, A_BOLD);

        int barw = (w > 60) ? 40 : w - 20;

        mvprintw(y, 2, "RAM  %5lu/%5lu MB  [",
                 (mem.mem_total_kb - mem.mem_free_kb)/1024,
                 mem.mem_total_kb/1024);
        draw_hbar(stdscr, y, 23, barw,
                  mem.mem_total_kb - mem.mem_free_kb,
                  mem.mem_total_kb, 1);
        mvprintw(y++, 23 + barw, "]");

        int swap_color = (mem.swap_used_kb > mem.swap_total_kb/2) ? 2 : 5;
        mvprintw(y, 2, "SWAP %5lu/%5lu MB  [",
                 mem.swap_used_kb/1024, mem.swap_total_kb/1024);
        draw_hbar(stdscr, y, 23, barw,
                  mem.swap_used_kb, mem.swap_total_kb > 0 ? mem.swap_total_kb : 1,
                  swap_color);
        mvprintw(y++, 23 + barw, "]");

        y++;

        /* Scan rates */
        wattron(stdscr, A_BOLD);
        mvprintw(y++, 2, "LRU SCAN RATES  (pages/sec)");
        wattroff(stdscr, A_BOLD);

        wattron(stdscr, COLOR_PAIR(1));
        mvprintw(y, 2, "kswapd bg   %8lu/s [", dk);
        wattroff(stdscr, COLOR_PAIR(1));
        draw_hbar(stdscr, y, 23, barw, dk, peak_kswapd > 0 ? peak_kswapd : 1, 1);
        mvprintw(y++, 23 + barw, "]");

        int dc = (dd > 0) ? 2 : 1;
        wattron(stdscr, COLOR_PAIR(dc));
        mvprintw(y, 2, "direct rec  %8lu/s [", dd);
        wattroff(stdscr, COLOR_PAIR(dc));
        draw_hbar(stdscr, y, 23, barw, dd, peak_direct > 0 ? peak_direct : 1, dc);
        mvprintw(y++, 23 + barw, "]");
        if (dd > 0) {
            wattron(stdscr, COLOR_PAIR(2) | A_BOLD);
            mvprintw(y, 23, "⚠  App threads doing reclaim work - raise vm.min_free_kbytes!");
            wattroff(stdscr, COLOR_PAIR(2) | A_BOLD);
        }
        y++;

        /* Steal rates */
        wattron(stdscr, A_BOLD);
        mvprintw(y++, 2, "PAGE STEAL RATES (pages/sec)");
        wattroff(stdscr, A_BOLD);
        mvprintw(y++, 2, "pgsteal_kswapd  %8lu/s    pgsteal_direct %8lu/s", dsk, dsd);
        y++;

        /* Swap I/O */
        wattron(stdscr, A_BOLD);
        mvprintw(y++, 2, "SWAP I/O  (pages/sec)");
        wattroff(stdscr, A_BOLD);

        int swin_c  = (dpi > 0) ? 5 : 7;
        int swout_c = (dpo > 0) ? 2 : 7;
        wattron(stdscr, COLOR_PAIR(swin_c));
        mvprintw(y++, 2, "swap-in   %8lu/s   (major faults: %lu/s)", dpi, dmf);
        wattroff(stdscr, COLOR_PAIR(swin_c));
        wattron(stdscr, COLOR_PAIR(swout_c));
        mvprintw(y, 2,   "swap-out  %8lu/s [", dpo);
        wattroff(stdscr, COLOR_PAIR(swout_c));
        draw_hbar(stdscr, y, 23, barw, dpo, peak_pswpout > 0 ? peak_pswpout : 1, swout_c);
        mvprintw(y++, 23 + barw, "]");
        y++;

        /* Sparkline history */
        wattron(stdscr, A_BOLD);
        mvprintw(y++, 2, "SCAN HISTORY  (last %d seconds,  ▄=kswapd  ▀=direct)", HISTORY);
        wattroff(stdscr, A_BOLD);

        int spark_w = (HISTORY < w - 4) ? HISTORY : w - 4;
        uint64_t pk = peak_kswapd > 0 ? peak_kswapd : 1;
        uint64_t pd = peak_direct > 0 ? peak_direct : 1;
        for (int i = 0; i < spark_w; i++) {
            int idx = (hist_idx - spark_w + i + HISTORY) % HISTORY;
            int hk = (int)(hist_kswapd[idx] * 4 / pk);
            int hd = (int)(hist_direct[idx] * 4 / pd);
            if (hk > 4) hk = 4;
            if (hd > 4) hd = 4;
            static const char *blocks[] = {" ","▁","▂","▃","▄"};
            wattron(stdscr, COLOR_PAIR(1));
            mvprintw(y, 2 + i, "%s", blocks[hk]);
            wattroff(stdscr, COLOR_PAIR(1));
            (void)hd;
        }
        y += 2;

        /* Recommendation */
        wattron(stdscr, COLOR_PAIR(4));
        mvhline(y++, 0, ACS_HLINE, cols);
        char *rec;
        uint64_t sw = (uint64_t)atoi(g_swappiness);
        if      (sw == 0)       rec = "swappiness=0: OOM risk. Consider swappiness=1-10.";
        else if (sw <= 10)      rec = "swappiness≤10: Good for DB/Redis. Minimal anon scanning.";
        else if (sw <= 30)      rec = "swappiness≤30: Good for JVM/app servers. Balanced.";
        else if (sw == 60)      rec = "swappiness=60 (default): Fine for desktops, aggressive for servers.";
        else                    rec = "swappiness>60: Actively prefers anon eviction. Lower for servers.";
        wattron(stdscr, A_BOLD);
        mvprintw(y, 2, "→ %s", rec);
        wattroff(stdscr, A_BOLD | COLOR_PAIR(4));

        refresh();
        prev = cur;
    }

    endwin();
    printf("\nMonitor exited cleanly.\n");
    return EXIT_SUCCESS;
}
