/* policy_engine.c — eBPF policy engine loader + ncurses monitor
 * Requires: libbpf >= 0.5, ncursesw, Linux 5.10+
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <locale.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ncurses.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#define VERDICT_ALLOW  1
#define VERDICT_DENY   0
#define MAX_DISP_EVTS  20
#define BPF_OBJ        "./policy_engine.bpf.o"
#define BPF_PIN        "/sys/fs/bpf/policy_engine_prog"

/* ── Structs (must mirror BPF) ─────────────────────────────────── */
struct policy_key   { uint32_t src_ip; uint16_t dst_port; uint8_t protocol; uint8_t pad; };
struct policy_entry { uint8_t action; uint8_t _pad[3]; uint32_t hits; };
struct pkt_event    { uint32_t src_ip, dst_ip; uint16_t src_port, dst_port;
                      uint8_t protocol, action; uint16_t _pad; uint64_t ts_ns; };

struct disp_evt {
    char     tstr[12];
    char     sip[INET_ADDRSTRLEN], dip[INET_ADDRSTRLEN];
    uint16_t sport, dport;
    uint8_t  proto, action;
};

/* ── Global state ─────────────────────────────────────────────── */
static volatile sig_atomic_t running  = 1;
static struct disp_evt        devts[MAX_DISP_EVTS];
static int                    ehead   = 0;
static int                    ecount  = 0;
static uint64_t               n_allow = 0, n_deny = 0;
static uint64_t               p_allow = 0, p_deny = 0;
static pthread_mutex_t        emtx    = PTHREAD_MUTEX_INITIALIZER;

static int                    pmap_fd = -1;
static struct ring_buffer    *rb      = NULL;
static const char            *iface   = "veth-policy";
static time_t                 t_start;

/* ── Helpers ──────────────────────────────────────────────────── */
static int quiet_print(enum libbpf_print_level l, const char *f, va_list a)
    { (void)l; (void)f; (void)a; return 0; }

static void on_signal(int s) { (void)s; running = 0; }

static const char *proto_name(uint8_t p) {
    switch (p) { case 1: return "ICMP"; case 6: return "TCP "; case 17: return "UDP "; }
    return "????";
}

/* ── Ring buffer callback ─────────────────────────────────────── */
static int handle_event(void *ctx, void *data, size_t len)
{
    (void)ctx; (void)len;
    struct pkt_event *ev = data;
    pthread_mutex_lock(&emtx);

    struct disp_evt *d = &devts[ehead % MAX_DISP_EVTS];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(d->tstr, sizeof(d->tstr), "%H:%M:%S", tm);

    struct in_addr sa = { .s_addr = ev->src_ip };
    struct in_addr da = { .s_addr = ev->dst_ip };
    inet_ntop(AF_INET, &sa, d->sip, sizeof(d->sip));
    inet_ntop(AF_INET, &da, d->dip, sizeof(d->dip));

    d->sport = ev->src_port; d->dport = ev->dst_port;
    d->proto = ev->protocol; d->action = ev->action;
    ehead++;
    if (ecount < MAX_DISP_EVTS) ecount++;
    if (ev->action == VERDICT_ALLOW) n_allow++; else n_deny++;

    pthread_mutex_unlock(&emtx);
    return 0;
}

/* ── ncurses UI ───────────────────────────────────────────────── */
static void draw(WINDOW *w, int rows, int cols)
{
    werase(w);

    /* Header bar */
    wattron(w, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
    mvwhline(w, 0, 0, ' ', cols);
    mvwprintw(w, 0, 2, " eBPF Policy Engine Monitor");
    mvwprintw(w, 0, cols - 30, "iface: %-12s PID:%-5d", iface, getpid());
    wattroff(w, COLOR_PAIR(3) | A_BOLD | A_REVERSE);

    /* Stats row */
    time_t now = time(NULL);
    int up = (int)(now - t_start);
    double rate = (double)(n_allow + n_deny - p_allow - p_deny);   /* per-tick approx */

    mvwprintw(w, 2, 2, "  Allowed: ");
    wattron(w, COLOR_PAIR(1) | A_BOLD);
    wprintw(w, "%-8llu", (unsigned long long)n_allow);
    wattroff(w, COLOR_PAIR(1) | A_BOLD);
    wprintw(w, "  Denied: ");
    wattron(w, COLOR_PAIR(2) | A_BOLD);
    wprintw(w, "%-6llu", (unsigned long long)n_deny);
    wattroff(w, COLOR_PAIR(2) | A_BOLD);
    wattron(w, COLOR_PAIR(5));
    wprintw(w, "  Rate: %5.1f pkt/s   Uptime: %02d:%02d", rate, up / 60, up % 60);
    wattroff(w, COLOR_PAIR(5));
    p_allow = n_allow; p_deny = n_deny;

    /* Events table */
    mvwhline(w, 3, 0, ACS_HLINE, cols);
    wattron(w, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(w, 4, 2, "RECENT EVENTS");
    wattroff(w, COLOR_PAIR(3) | A_BOLD);
    wattron(w, A_UNDERLINE);
    mvwprintw(w, 5, 2, "%-10s %-5s %-17s %-17s %5s  %-9s",
              "TIME", "PROTO", "SOURCE", "DESTINATION", "DPORT", "VERDICT");
    wattroff(w, A_UNDERLINE);

    int max_rows = (rows - 14 > 0) ? rows - 14 : 4;
    if (max_rows > MAX_DISP_EVTS) max_rows = MAX_DISP_EVTS;

    pthread_mutex_lock(&emtx);
    for (int i = 0; i < ecount && i < max_rows; i++) {
        int idx = ((ehead - 1 - i) % MAX_DISP_EVTS + MAX_DISP_EVTS) % MAX_DISP_EVTS;
        struct disp_evt *d = &devts[idx];
        mvwprintw(w, 6 + i, 2, "%-10s %-5s %-17s %-17s %5u  ",
                  d->tstr, proto_name(d->proto), d->sip, d->dip, d->dport);
        if (d->action == VERDICT_ALLOW) {
            wattron(w, COLOR_PAIR(1) | A_BOLD);
            wprintw(w, "[ALLOW]");
            wattroff(w, COLOR_PAIR(1) | A_BOLD);
        } else {
            wattron(w, COLOR_PAIR(2) | A_BOLD);
            wprintw(w, "[ DENY]");
            wattroff(w, COLOR_PAIR(2) | A_BOLD);
        }
    }
    pthread_mutex_unlock(&emtx);

    /* Policy table */
    int pr = rows - 8;
    if (pr < 6) pr = 6;
    mvwhline(w, pr, 0, ACS_HLINE, cols);
    wattron(w, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(w, pr + 1, 2, "ACTIVE POLICIES  (initial: deny TCP/8080, allow ICMP from peer)");
    wattroff(w, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(w, pr + 2, 2, "  #1  src=0.0.0.0 (any)      port=8080  proto=TCP   action= ");
    wattron(w, COLOR_PAIR(2) | A_BOLD); wprintw(w, "DENY"); wattroff(w, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(w, pr + 3, 2, "  #2  src=%-15s  port=0     proto=ICMP  action= ", "10.99.0.2");
    wattron(w, COLOR_PAIR(1) | A_BOLD); wprintw(w, "ALLOW"); wattroff(w, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(w, pr + 4, 2, "  #3  (default)                                   action= ");
    wattron(w, COLOR_PAIR(1) | A_BOLD); wprintw(w, "ALLOW"); wattroff(w, COLOR_PAIR(1) | A_BOLD);

    /* Footer */
    wattron(w, A_REVERSE | COLOR_PAIR(4));
    mvwhline(w, rows - 1, 0, ' ', cols);
    mvwprintw(w, rows - 1, 2,
              "[A] Allow TCP/8080   [D] Deny TCP/8080   [C] Clear rules   [Q] Quit");
    wattroff(w, A_REVERSE | COLOR_PAIR(4));

    wrefresh(w);
}

/* ── Policy helpers ───────────────────────────────────────────── */
static void policy_set(uint32_t src, uint16_t port, uint8_t proto, uint8_t act)
{
    struct policy_key   k = { .src_ip = src, .dst_port = port,
                               .protocol = proto, .pad = 0 };
    struct policy_entry e = { .action = act, ._pad = {0,0,0}, .hits = 0 };
    bpf_map_update_elem(pmap_fd, &k, &e, BPF_ANY);
}

static void policy_clear(void)
{
    struct policy_key nk;
    while (bpf_map_get_next_key(pmap_fd, NULL, &nk) == 0)
        bpf_map_delete_elem(pmap_fd, &nk);
}

/* ── main ─────────────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    if (argc > 1) iface = argv[1];
    t_start = time(NULL);

    setlocale(LC_ALL, "");
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    libbpf_set_print(quiet_print);

    /* Load BPF object */
    struct bpf_object *obj = bpf_object__open(BPF_OBJ);
    if (!obj) { perror("bpf_object__open"); return 1; }

    /* Explicitly set program type for libbpf 0.5 compatibility */
    struct bpf_program *prog = bpf_object__find_program_by_name(obj, "policy_engine_fn");
    if (!prog) { fprintf(stderr, "BPF prog not found\n"); return 1; }
    bpf_program__set_type(prog, BPF_PROG_TYPE_SCHED_CLS);

    if (bpf_object__load(obj)) { perror("bpf_object__load"); return 1; }

    /* Pin program for tc filter attach */
    unlink(BPF_PIN);
    if (bpf_program__pin(prog, BPF_PIN)) { perror("bpf_program__pin"); return 1; }

    /* Find maps */
    struct bpf_map *pm = bpf_object__find_map_by_name(obj, "policy_map");
    struct bpf_map *em = bpf_object__find_map_by_name(obj, "events");
    if (!pm || !em) { fprintf(stderr, "BPF maps not found\n"); return 1; }

    pmap_fd = bpf_map__fd(pm);
    int efd  = bpf_map__fd(em);

    /* Attach TC filter via system tc command */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "tc qdisc add dev %s clsact 2>/dev/null; "
        "tc filter del dev %s ingress 2>/dev/null; "
        "tc filter add dev %s ingress bpf pinned %s direct-action",
        iface, iface, iface, BPF_PIN);
    if (system(cmd) != 0) {
        fprintf(stderr, "TC attach failed. Interface: %s\n", iface);
        return 1;
    }

    /* Install initial demo policies */
    struct in_addr peer; inet_pton(AF_INET, "10.99.0.2", &peer);
    policy_set(0,          8080, 6, VERDICT_DENY);   /* deny TCP/8080 any src */
    policy_set(peer.s_addr, 0,  1, VERDICT_ALLOW);   /* allow ICMP from peer  */

    /* Set up ring buffer consumer */
    rb = ring_buffer__new(efd, handle_event, NULL, NULL);
    if (!rb) { perror("ring_buffer__new"); return 1; }

    /* ncurses setup */
    initscr();
    start_color();
    cbreak(); noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    init_pair(1, COLOR_GREEN,  COLOR_BLACK);
    init_pair(2, COLOR_RED,    COLOR_BLACK);
    init_pair(3, COLOR_CYAN,   COLOR_BLACK);
    init_pair(4, COLOR_WHITE,  COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);

    WINDOW *win = newwin(LINES, COLS, 0, 0);

    /* Main loop */
    while (running) {
        ring_buffer__poll(rb, 100);
        draw(win, LINES, COLS);

        int ch = getch();
        switch (ch) {
            case 'q': case 'Q': running = 0; break;
            case 'a': case 'A': policy_set(0, 8080, 6, VERDICT_ALLOW); break;
            case 'd': case 'D': policy_set(0, 8080, 6, VERDICT_DENY);  break;
            case 'c': case 'C': policy_clear(); break;
        }
    }

    endwin();

    /* Teardown */
    ring_buffer__free(rb);

    snprintf(cmd, sizeof(cmd),
        "tc filter del dev %s ingress 2>/dev/null; "
        "tc qdisc del dev %s clsact 2>/dev/null",
        iface, iface);
    if (system(cmd) != 0) { /* best-effort teardown */ }
    unlink(BPF_PIN);
    bpf_object__close(obj);

    printf("\nPolicy engine stopped. Allowed: %llu  Denied: %llu\n",
           (unsigned long long)n_allow, (unsigned long long)n_deny);
    return 0;
}
