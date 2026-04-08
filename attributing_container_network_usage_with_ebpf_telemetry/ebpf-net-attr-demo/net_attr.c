#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <ncurses.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#define MAX_CONTAINERS  128
#define CGPATH          "/sys/fs/cgroup"
#define STATS_PIN       "/sys/fs/bpf/net_attr_stats"
#define EVENTS_PIN      "/sys/fs/bpf/net_attr_events"
#define REFRESH_MS      1000
#define MAX_KEYS        512

struct net_stats {
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t tx_packets;
};

struct cg_entry {
    uint64_t cgroupid;
    char     name[96];
    int      active;
};

struct snap_entry {
    uint64_t     cgid;
    struct net_stats total;
};

static struct cg_entry  g_cache[MAX_CONTAINERS];
static int              g_ncache = 0;
static pthread_mutex_t  g_cache_lock = PTHREAD_MUTEX_INITIALIZER;
static struct snap_entry g_prev[MAX_KEYS];
static struct snap_entry g_curr[MAX_KEYS];
static int               g_prev_n = 0, g_curr_n = 0;
static int               g_map_fd = -1, g_ncpus = 0;
static volatile int      g_quit = 0;

static void sig_handler(int sig) { (void)sig; g_quit = 1; }

static void parse_cg_name(const char *d, char *out)
{
    const char *p;
    if ((p = strstr(d, "docker-")))    { p += 7; snprintf(out, 96, "%.12s", p); return; }
    if ((p = strstr(d, "cri-containerd-"))) { p += 15; snprintf(out, 96, "ctr:%.10s", p); return; }
    snprintf(out, 96, "%.22s", d);
}

static void walk_cgroups(const char *base, int depth)
{
    if (depth > 8) return;
    DIR *d = opendir(base);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_type != DT_DIR || ent->d_name[0] == '.') continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", base, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if ((strstr(ent->d_name, "docker-") || strstr(ent->d_name, "cri-containerd-"))
                && g_ncache < MAX_CONTAINERS) {
            pthread_mutex_lock(&g_cache_lock);
            int found = 0;
            for (int i = 0; i < g_ncache; i++)
                if (g_cache[i].cgroupid == (uint64_t)st.st_ino) { found=1; break; }
            if (!found) {
                g_cache[g_ncache].cgroupid = (uint64_t)st.st_ino;
                g_cache[g_ncache].active   = 1;
                parse_cg_name(ent->d_name, g_cache[g_ncache].name);
                g_ncache++;
            }
            pthread_mutex_unlock(&g_cache_lock);
        }
        walk_cgroups(path, depth + 1);
    }
    closedir(d);
}

static const char *resolve(uint64_t cgid)
{
    pthread_mutex_lock(&g_cache_lock);
    for (int i = 0; i < g_ncache; i++)
        if (g_cache[i].cgroupid == cgid) { pthread_mutex_unlock(&g_cache_lock); return g_cache[i].name; }
    pthread_mutex_unlock(&g_cache_lock);
    return NULL;
}

static int read_stats(uint64_t cgid, struct net_stats *out)
{
    struct net_stats *percpu = calloc((size_t)g_ncpus, sizeof(*percpu));
    if (!percpu) return -1;
    int r = bpf_map_lookup_elem(g_map_fd, &cgid, percpu);
    if (r == 0) {
        memset(out, 0, sizeof(*out));
        for (int i = 0; i < g_ncpus; i++) {
            out->rx_bytes   += percpu[i].rx_bytes;
            out->tx_bytes   += percpu[i].tx_bytes;
            out->rx_packets += percpu[i].rx_packets;
            out->tx_packets += percpu[i].tx_packets;
        }
    }
    free(percpu);
    return r;
}

static void collect_snapshot(void)
{
    g_curr_n = 0;
    uint64_t key = 0, nk = 0;
    while (bpf_map_get_next_key(g_map_fd, (g_curr_n==0)?NULL:&key, &nk)==0
           && g_curr_n < MAX_KEYS) {
        key = nk;
        struct net_stats t = {};
        if (read_stats(key, &t) == 0) { g_curr[g_curr_n].cgid=key; g_curr[g_curr_n].total=t; g_curr_n++; }
    }
}

static void get_delta(uint64_t cgid, const struct net_stats *cur, struct net_stats *d)
{
    for (int i = 0; i < g_prev_n; i++) {
        if (g_prev[i].cgid == cgid) {
            d->rx_bytes=cur->rx_bytes-g_prev[i].total.rx_bytes;
            d->tx_bytes=cur->tx_bytes-g_prev[i].total.tx_bytes;
            d->rx_packets=cur->rx_packets-g_prev[i].total.rx_packets;
            d->tx_packets=cur->tx_packets-g_prev[i].total.tx_packets;
            return;
        }
    }
    memset(d, 0, sizeof(*d));
}

static void fmt_rate(char *b, size_t n, uint64_t Bps)
{
    if (Bps >= 1ULL<<30)       snprintf(b,n,"%.1f GB/s",(double)Bps/(1ULL<<30));
    else if (Bps >= 1ULL<<20)  snprintf(b,n,"%5.1f MB/s",(double)Bps/(1ULL<<20));
    else if (Bps >= 1024ULL)   snprintf(b,n,"%5.1f KB/s",(double)Bps/1024.0);
    else                        snprintf(b,n,"%5" PRIu64 "  B/s",Bps);
}

static void draw(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();
    attron(COLOR_PAIR(1)|A_BOLD);
    for(int c=0;c<cols;c++) mvaddch(0,c,' ');
    mvprintw(0,2,"  eBPF Container Network Attribution  |  TC clsact + PERCPU_HASH  |  cgroups v2");
    attroff(COLOR_PAIR(1)|A_BOLD);
    attron(COLOR_PAIR(2)|A_BOLD);
    mvprintw(2,2,"%-22s %-18s %-13s %-13s %10s %10s","CONTAINER","CGROUPID","RX","TX","RX PKT/s","TX PKT/s");
    attroff(COLOR_PAIR(2)|A_BOLD);
    attron(COLOR_PAIR(3));
    mvhline(3,2,ACS_HLINE,cols-4);
    attroff(COLOR_PAIR(3));

    int row=4;
    for(int i=0;i<g_curr_n&&row<rows-3;i++){
        uint64_t cgid=g_curr[i].cgid;
        struct net_stats delta;
        get_delta(cgid,&g_curr[i].total,&delta);
        const char *nm=resolve(cgid);
        char nb[24]; if(nm) snprintf(nb,24,"%.21s",nm); else snprintf(nb,24,"<%" PRIx64 ">",cgid);
        char rx[20],tx[20];
        fmt_rate(rx,sizeof(rx),delta.rx_bytes);
        fmt_rate(tx,sizeof(tx),delta.tx_bytes);
        int hi=(delta.rx_bytes>10<<20||delta.tx_bytes>10<<20);
        attron(hi?COLOR_PAIR(4)|A_BOLD:COLOR_PAIR(5));
        mvprintw(row,2,"%-22s %-18" PRIu64 " %-13s %-13s %10" PRIu64 " %10" PRIu64,
                 nb,cgid,rx,tx,delta.rx_packets,delta.tx_packets);
        attroff(hi?COLOR_PAIR(4)|A_BOLD:COLOR_PAIR(5));
        row++;
    }

    if(g_curr_n==0){
        attron(COLOR_PAIR(6));
        mvprintw(5,4,"No container traffic visible yet — waiting for first packets...");
        mvprintw(6,4,"(eBPF only sees traffic on veth interfaces where TC hooks are attached)");
        attroff(COLOR_PAIR(6));
    }

    attron(COLOR_PAIR(1));
    for(int c=0;c<cols;c++) mvaddch(rows-2,c,' ');
    mvprintw(rows-2,2,"q:quit  r:refresh cache  entries:%-3d  cpus:%-2d  map:%s",g_curr_n,g_ncpus,STATS_PIN);
    attroff(COLOR_PAIR(1));
    refresh();
}

static void print_once_snapshot(void)
{
    collect_snapshot();
    printf("%-22s %-18s %12s %12s %10s %10s\n",
           "CONTAINER","CGROUPID","RX_BYTES","TX_BYTES","RX_PKTS","TX_PKTS");
    for (int i = 0; i < g_curr_n; i++) {
        uint64_t cgid = g_curr[i].cgid;
        const char *nm = resolve(cgid);
        char nb[24];
        if (nm) snprintf(nb, 24, "%.21s", nm);
        else snprintf(nb, 24, "<%016" PRIx64 ">", cgid);
        printf("%-22s %-18" PRIu64 " %12" PRIu64 " %12" PRIu64 " %10" PRIu64 " %10" PRIu64 "\n",
               nb, cgid,
               g_curr[i].total.rx_bytes, g_curr[i].total.tx_bytes,
               g_curr[i].total.rx_packets, g_curr[i].total.tx_packets);
    }
    if (g_curr_n == 0)
        printf("(no cgroup keys in map yet — run traffic through hooked veth)\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    const char *sp = STATS_PIN;
    int once_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--once") == 0)
            once_mode = 1;
        else if (argv[i][0] != '-')
            sp = argv[i];
    }
    signal(SIGINT,sig_handler); signal(SIGTERM,sig_handler);
    libbpf_set_print(NULL);
    g_ncpus=libbpf_num_possible_cpus();
    if(g_ncpus<=0){perror("num_possible_cpus");return 1;}
    g_map_fd=bpf_obj_get(sp);
    if(g_map_fd<0){fprintf(stderr,"Cannot open %s: %s\nRun setup.sh first.\n",sp,strerror(errno));return 1;}
    walk_cgroups(CGPATH,0);
    if (once_mode) {
        print_once_snapshot();
        close(g_map_fd);
        return 0;
    }
    initscr(); cbreak(); noecho(); keypad(stdscr,TRUE); nodelay(stdscr,TRUE); curs_set(0);
    start_color(); use_default_colors();
    init_pair(1,COLOR_BLACK,COLOR_BLUE);
    init_pair(2,COLOR_CYAN,-1);
    init_pair(3,COLOR_BLUE,-1);
    init_pair(4,COLOR_YELLOW,-1);
    init_pair(5,COLOR_WHITE,-1);
    init_pair(6,COLOR_RED,-1);

    struct timespec ts_next;
    clock_gettime(CLOCK_MONOTONIC,&ts_next);

    while(!g_quit){
        int ch=getch();
        if(ch=='q'||ch=='Q') break;
        if(ch=='r'||ch=='R') walk_cgroups(CGPATH,0);
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC,&now);
        long diff=(now.tv_sec-ts_next.tv_sec)*1000L+(now.tv_nsec-ts_next.tv_nsec)/1000000L;
        if(diff>=0){
            memcpy(g_prev,g_curr,sizeof(g_curr)); g_prev_n=g_curr_n;
            collect_snapshot(); draw();
            ts_next.tv_sec+=REFRESH_MS/1000; ts_next.tv_nsec+=(REFRESH_MS%1000)*1000000L;
            if(ts_next.tv_nsec>=1000000000L){ts_next.tv_sec++;ts_next.tv_nsec-=1000000000L;}
        }
        usleep(50000);
    }
    endwin();
    close(g_map_fd);
    printf("Done.\n");
    return 0;
}
