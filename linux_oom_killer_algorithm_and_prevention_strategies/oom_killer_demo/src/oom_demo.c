/*
 * oom_demo.c — Linux OOM Killer: heuristics & scoring demonstration
 *
 * Demonstrates:
 *  - Reading /proc/self/oom_score and oom_score_adj
 *  - Effect of RSS growth on oom_score (allocate+touch pages)
 *  - oom_score_adj manipulation
 *  - PSI (Pressure Stall Information) from /proc/pressure/memory
 *  - CommitLimit / Committed_AS from /proc/meminfo
 *  - Top-N processes sorted by oom_score
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -g -o bin/oom_demo src/oom_demo.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <dirent.h>
#include <ctype.h>

/* ── helpers ─────────────────────────────────────────────── */

static int read_proc_long(const char *path, long *out)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int ret = (fscanf(f, "%ld", out) == 1) ? 0 : -1;
    fclose(f);
    return ret;
}

/* Read named field from /proc/meminfo, value in kB */
static long meminfo_key(const char *key)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;
    char line[256], kname[64];
    long val = -1;
    while (fgets(line, sizeof(line), f)) {
        long v;
        if (sscanf(line, "%63[^:]: %ld", kname, &v) == 2) {
            if (strcmp(kname, key) == 0) { val = v; break; }
        }
    }
    fclose(f);
    return val;
}

static void print_separator(const char *title)
{
    printf("\n\033[1;34m══════ %s ══════\033[0m\n", title);
}

/* ── OOM score reader ─────────────────────────────────────── */

static void show_self_oom_score(void)
{
    long score = -1, adj = -1;
    if (read_proc_long("/proc/self/oom_score", &score) == 0 &&
        read_proc_long("/proc/self/oom_score_adj", &adj) == 0) {
        printf("  \033[1mThis process\033[0m   PID %-6d   "
               "oom_score: \033[1;33m%3ld\033[0m   "
               "oom_score_adj: \033[1;32m%d\033[0m\n",
               getpid(), score, (int)adj);
    }
}

/* ── Memory allocation experiment ────────────────────────── */

#define CHUNK_SIZE   (16UL * 1024 * 1024)  /* 16 MiB per step */
#define MAX_CHUNKS   8

static void *alloc_chunks[MAX_CHUNKS];
static int   alloc_count = 0;

/* Touch every page to force RSS growth (not just VSZ) */
static void touch_pages(void *p, size_t sz)
{
    volatile char *ptr = (volatile char *)p;
    for (size_t i = 0; i < sz; i += 4096)
        ptr[i] = (char)(i & 0xff);
}

static void allocation_experiment(void)
{
    print_separator("RSS Growth → oom_score Changes");
    printf("  Allocating %d × %lu MiB chunks, touching all pages\n",
           MAX_CHUNKS, CHUNK_SIZE / (1024 * 1024));
    printf("  %-6s  %-12s  %-12s  %-12s\n",
           "Chunk", "Alloc (MiB)", "oom_score", "adj");
    printf("  %-6s  %-12s  %-12s  %-12s\n",
           "─────", "──────────", "─────────", "───");

    for (int i = 0; i < MAX_CHUNKS; i++) {
        void *p = mmap(NULL, CHUNK_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            fprintf(stderr, "  mmap failed at chunk %d: %s\n",
                    i, strerror(errno));
            break;
        }
        touch_pages(p, CHUNK_SIZE);
        alloc_chunks[alloc_count++] = p;

        long score = -1, adj = -1;
        (void)read_proc_long("/proc/self/oom_score", &score);
        (void)read_proc_long("/proc/self/oom_score_adj", &adj);

        const char *color = score > 400 ? "\033[1;31m" :
                            score > 200 ? "\033[1;33m" : "\033[1;32m";
        printf("  %-6d  %-12lu  %s%-12ld\033[0m  %-12ld\n",
               i + 1,
               (unsigned long)(CHUNK_SIZE * (size_t)(alloc_count) / (1024 * 1024)),
               color, score, adj);
    }
}

/* ── oom_score_adj demo ───────────────────────────────────── */

static void adj_experiment(void)
{
    print_separator("oom_score_adj Effect");
    printf("  Writing to /proc/self/oom_score_adj requires\n");
    printf("  CAP_SYS_RESOURCE (available in privileged container)\n\n");

    struct { int adj; const char *label; } tests[] = {
        { 0,    "default (no bias)"       },
        { 300,  "moderate penalty"        },
        { -500, "protected (discouraged)" },
        { -1000,"immune (LONG_MIN)"       },
        { 0,    "restored to default"     },
    };
    int n = (int)(sizeof(tests) / sizeof(tests[0]));

    for (int i = 0; i < n; i++) {
        FILE *f = fopen("/proc/self/oom_score_adj", "w");
        if (!f) {
            warn_no_cap:
            printf("  \033[33m⚠ Cannot write oom_score_adj — "
                   "need CAP_SYS_RESOURCE\033[0m\n");
            printf("  Run demo container with --privileged flag\n");
            return;
        }
        if (fprintf(f, "%d\n", tests[i].adj) < 0) {
            fclose(f);
            goto warn_no_cap;
        }
        fclose(f);

        long score = -1, adj_r = -1;
        (void)read_proc_long("/proc/self/oom_score", &score);
        (void)read_proc_long("/proc/self/oom_score_adj", &adj_r);

        printf("  adj = \033[1m%+5d\033[0m  →  oom_score: \033[1;33m%3ld\033[0m"
               "   (%s)\n",
               tests[i].adj, score, tests[i].label);
        usleep(50000);
    }
    return;

    /* Suppress unused-label warning */
    (void)0;
    goto warn_no_cap;
}

/* ── PSI reader ───────────────────────────────────────────── */

static void show_psi(void)
{
    print_separator("PSI: /proc/pressure/memory  (kernel 4.20+)");

    FILE *f = fopen("/proc/pressure/memory", "r");
    if (!f) {
        printf("  \033[33m⚠ /proc/pressure/memory not available\033[0m\n");
        printf("  Requires kernel >= 4.20 and CONFIG_PSI=y\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        /* Remove trailing newline */
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "some", 4) == 0) {
            printf("  \033[1;36msome\033[0m %s\n", line + 5);
            printf("       ↑ at least one task stalled (early warning)\n");
        } else if (strncmp(line, "full", 4) == 0) {
            printf("  \033[1;31mfull\033[0m %s\n", line + 5);
            printf("       ↑ ALL tasks stalled (critical — act now)\n");
        }
    }
    fclose(f);
}

/* ── Meminfo snapshot ─────────────────────────────────────── */

static void show_meminfo(void)
{
    print_separator("Memory Commit State  (/proc/meminfo)");

    long mem_total    = meminfo_key("MemTotal");
    long mem_avail    = meminfo_key("MemAvailable");
    long commit_limit = meminfo_key("CommitLimit");
    long committed    = meminfo_key("Committed_AS");
    long swap_total   = meminfo_key("SwapTotal");
    long swap_free    = meminfo_key("SwapFree");

    if (mem_total > 0) {
        printf("  MemTotal:       %8ld MiB\n", mem_total / 1024);
        printf("  MemAvailable:   %8ld MiB  (reclaimable + free)\n",
               mem_avail / 1024);
        printf("  CommitLimit:    %8ld MiB  (overcommit ceiling)\n",
               commit_limit / 1024);

        const char *color = (committed > commit_limit) ? "\033[1;31m" : "\033[0m";
        printf("  Committed_AS:  %s%8ld MiB\033[0m  (total promised)\n",
               color, committed / 1024);
        if (committed > commit_limit)
            printf("  \033[1;31m  ⚠ Committed > CommitLimit — past overcommit boundary\033[0m\n");

        printf("  SwapTotal:      %8ld MiB\n", swap_total / 1024);
        printf("  SwapFree:       %8ld MiB\n", swap_free / 1024);
    } else {
        printf("  \033[33m⚠ Could not read /proc/meminfo\033[0m\n");
    }
}

/* ── Top processes by oom_score ───────────────────────────── */

#define MAX_PROCS 1024

typedef struct {
    int  pid;
    int  score;
    int  adj;
    long rss_kib;
    char name[64];
} ProcEntry;

static ProcEntry proc_table[MAX_PROCS];
static int proc_count = 0;

static int cmp_score_desc(const void *a, const void *b)
{
    return ((const ProcEntry *)b)->score - ((const ProcEntry *)a)->score;
}

static int is_numeric(const char *s)
{
    if (!*s) return 0;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s)) return 0;
    return 1;
}

static void scan_top_procs(void)
{
    print_separator("Top Processes by oom_score");

    DIR *d = opendir("/proc");
    if (!d) { printf("  Cannot open /proc\n"); return; }

    proc_count = 0;
    struct dirent *ent;

    while ((ent = readdir(d)) != NULL && proc_count < MAX_PROCS) {
        if (!is_numeric(ent->d_name)) continue;

        int pid = atoi(ent->d_name);
        char path[128];
        long score = 0, adj = 0;

        snprintf(path, sizeof(path), "/proc/%d/oom_score", pid);
        if (read_proc_long(path, &score) < 0) continue;

        snprintf(path, sizeof(path), "/proc/%d/oom_score_adj", pid);
        (void)read_proc_long(path, &adj);

        /* Read name and RSS from /proc/[pid]/status */
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;

        char name[64] = "?";
        long rss_kib = 0;
        char line[256];

        while (fgets(line, sizeof(line), f)) {
            char key[64];
            if (sscanf(line, "%63[^:]:", key) != 1) continue;
            if (strcmp(key, "Name") == 0)
                sscanf(line, "%*[^:]: %63s", name);
            else if (strcmp(key, "VmRSS") == 0)
                sscanf(line, "%*[^:]: %ld", &rss_kib);
        }
        fclose(f);

        proc_table[proc_count].pid     = pid;
        proc_table[proc_count].score   = (int)score;
        proc_table[proc_count].adj     = (int)adj;
        proc_table[proc_count].rss_kib = rss_kib;
        memcpy(proc_table[proc_count].name, name, 63);
        proc_table[proc_count].name[63] = '\0';
        proc_count++;
    }
    closedir(d);

    qsort(proc_table, (size_t)proc_count, sizeof(ProcEntry), cmp_score_desc);

    int show = (proc_count < 15) ? proc_count : 15;
    printf("  %-6s  %-20s  %-10s  %-8s  %-10s\n",
           "PID", "NAME", "oom_score", "adj", "RSS(MiB)");
    printf("  %-6s  %-20s  %-10s  %-8s  %-10s\n",
           "──────", "────────────────────",
           "─────────", "───────", "──────────");

    for (int i = 0; i < show; i++) {
        const ProcEntry *p = &proc_table[i];
        const char *color = p->score > 400 ? "\033[1;31m" :
                            p->score > 200 ? "\033[1;33m" : "\033[1;32m";
        printf("  %-6d  %-20.20s  %s%-10d\033[0m  %-8d  %-10ld\n",
               p->pid, p->name, color, p->score,
               p->adj, p->rss_kib / 1024);
    }

    printf("\n  Color key:  \033[1;32m●\033[0m low (<200)"
           "   \033[1;33m●\033[0m medium (200-400)"
           "   \033[1;31m●\033[0m high (>400)\n");
}

/* ── OOM kill log hint ────────────────────────────────────── */

static void show_dmesg_hint(void)
{
    print_separator("Diagnosing Past OOM Events");
    printf("  Post-mortem in kernel ring buffer:\n\n");
    printf("  \033[1;36m$ dmesg | grep -A 40 'oom-killer'\033[0m\n\n");
    printf("  Key fields to read:\n");
    printf("  \033[1morder=0\033[0m         — single 4KB page failed; system genuinely full\n");
    printf("  \033[1mgfp_mask\033[0m        — allocation context (GFP_HIGHUSER = user page fault)\n");
    printf("  \033[1mactive_anon\033[0m     — hot anonymous pages; if low, reclaim was exhausted\n");
    printf("  \033[1minactive_anon\033[0m   — cold pages available for swap; if near 0 = crisis\n");
    printf("  \033[1mscore %d\033[0m       — victim's normalized score at kill time\n", 237);
    printf("  \033[1manon-rss\033[0m        — resident anonymous bytes (heap+stack) at kill\n");
    printf("\n  Example line:\n");
    printf("  \033[0;33m  Out of memory: Killed process 14392 (postgres) score 237\033[0m\n");
    printf("  \033[0;33m    total-vm:8388608kB, anon-rss:4194304kB, file-rss:32768kB\033[0m\n");
}

/* ── Cleanup mmap allocations ─────────────────────────────── */

static void cleanup_allocs(void)
{
    for (int i = 0; i < alloc_count; i++) {
        if (alloc_chunks[i])
            munmap(alloc_chunks[i], CHUNK_SIZE);
    }
}

static void sig_cleanup(int sig)
{
    cleanup_allocs();
    _exit(128 + sig);
}

/* ── main ────────────────────────────────────────────────── */

int main(void)
{
    signal(SIGINT,  sig_cleanup);
    signal(SIGTERM, sig_cleanup);

    printf("\033[1;34m");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║    Linux OOM Killer — Algorithm Deep Dive Demo       ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\033[0m");

    show_self_oom_score();
    show_meminfo();
    allocation_experiment();
    show_psi();
    adj_experiment();
    scan_top_procs();
    show_dmesg_hint();

    printf("\n\033[1;32m✓ Demo complete. All mmap allocations freed.\033[0m\n\n");
    cleanup_allocs();
    return 0;
}
