/*
 * swappiness_probe.c - Read kernel vm stats and swappiness value
 * Demonstrates: /proc/vmstat, /proc/sys/vm/swappiness parsing
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -o swappiness_probe swappiness_probe.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#define PROC_VMSTAT   "/proc/vmstat"
#define PROC_MEMINFO  "/proc/meminfo"
#define PROC_SWAPPY   "/proc/sys/vm/swappiness"
#define PROC_MIN_FREE "/proc/sys/vm/min_free_kbytes"
#define ZSWAP_EN      "/sys/module/zswap/parameters/enabled"
#define MGLRU_EN      "/sys/kernel/mm/lru_gen/enabled"

typedef struct {
    uint64_t pgscank;      /* kswapd page scans */
    uint64_t pgscand;      /* direct reclaim scans */
    uint64_t pswpin;       /* pages swapped in */
    uint64_t pswpout;      /* pages swapped out */
    uint64_t pgsteal_kswapd; /* pages stolen by kswapd */
    uint64_t pgsteal_direct; /* pages stolen via direct reclaim */
} VmStats;

typedef struct {
    uint64_t mem_total_kb;
    uint64_t mem_free_kb;
    uint64_t swap_total_kb;
    uint64_t swap_free_kb;
    uint64_t anon_pages_kb;
    uint64_t swap_cached_kb;
} MemInfo;

static int read_vmstats(VmStats *s) {
    FILE *f = fopen(PROC_VMSTAT, "r");
    if (!f) { perror("fopen " PROC_VMSTAT); return -1; }

    char key[64];
    uint64_t val;
    memset(s, 0, sizeof(*s));

    while (fscanf(f, "%63s %lu", key, &val) == 2) {
        if      (strcmp(key, "pgscank") == 0)         s->pgscank = val;
        else if (strcmp(key, "pgscand") == 0)         s->pgscand = val;
        else if (strcmp(key, "pswpin") == 0)          s->pswpin = val;
        else if (strcmp(key, "pswpout") == 0)         s->pswpout = val;
        else if (strcmp(key, "pgsteal_kswapd") == 0)  s->pgsteal_kswapd = val;
        else if (strcmp(key, "pgsteal_direct") == 0)  s->pgsteal_direct = val;
    }
    fclose(f);
    return 0;
}

static int read_meminfo(MemInfo *m) {
    FILE *f = fopen(PROC_MEMINFO, "r");
    if (!f) { perror("fopen " PROC_MEMINFO); return -1; }

    char key[64], unit[16];
    uint64_t val;
    memset(m, 0, sizeof(*m));

    while (fscanf(f, "%63s %lu %15s", key, &val, unit) >= 2) {
        if      (strcmp(key, "MemTotal:") == 0)    m->mem_total_kb  = val;
        else if (strcmp(key, "MemFree:") == 0)     m->mem_free_kb   = val;
        else if (strcmp(key, "SwapTotal:") == 0)   m->swap_total_kb = val;
        else if (strcmp(key, "SwapFree:") == 0)    m->swap_free_kb  = val;
        else if (strcmp(key, "AnonPages:") == 0)   m->anon_pages_kb = val;
        else if (strcmp(key, "SwapCached:") == 0)  m->swap_cached_kb = val;
    }
    fclose(f);
    return 0;
}

static int read_uint_file(const char *path, uint64_t *out) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int r = (fscanf(f, "%lu", out) == 1) ? 0 : -1;
    fclose(f);
    return r;
}

static void read_str_file(const char *path, char *buf, size_t n) {
    FILE *f = fopen(path, "r");
    if (!f) { snprintf(buf, n, "N/A"); return; }
    if (!fgets(buf, (int)n, f)) snprintf(buf, n, "N/A");
    else { size_t l = strlen(buf); if (l && buf[l-1] == '\n') buf[l-1] = '\0'; }
    fclose(f);
}

static void bar(uint64_t used, uint64_t total, int width) {
    if (total == 0) { printf("[%-*s]", width, ""); return; }
    int filled = (int)((double)used / total * width);
    printf("[");
    for (int i = 0; i < width; i++) putchar(i < filled ? '#' : '-');
    printf("]");
}

int main(void) {
    VmStats vs;
    MemInfo mi;
    uint64_t swappiness = 0, min_free_kb = 0;
    char zswap_en[8] = "N/A", mglru_en[16] = "N/A";

    if (read_vmstats(&vs) != 0)   return EXIT_FAILURE;
    if (read_meminfo(&mi) != 0)   return EXIT_FAILURE;

    read_uint_file(PROC_SWAPPY,   &swappiness);
    read_uint_file(PROC_MIN_FREE, &min_free_kb);
    read_str_file(ZSWAP_EN,  zswap_en, sizeof(zswap_en));
    read_str_file(MGLRU_EN,  mglru_en, sizeof(mglru_en));

    uint64_t swap_used_kb  = mi.swap_total_kb - mi.swap_free_kb;
    uint64_t mem_used_kb   = mi.mem_total_kb  - mi.mem_free_kb;

    printf("\n┌────────────────────────────────────────────────────┐\n");
    printf("│      vm.swappiness Deep Dive - System Snapshot      │\n");
    printf("├────────────────────────────────────────────────────┤\n");

    printf("│ vm.swappiness    : %-5lu  (anon_prio=%lu, file_prio=%lu)%s│\n",
           swappiness, swappiness, 200 - swappiness,
           swappiness > 9 ? "  " : "   ");

    printf("│ vm.min_free_kbytes: %-8lu  (~%lu MB)%*s│\n",
           min_free_kb, min_free_kb / 1024,
           (int)(7 - (min_free_kb/1024 > 99 ? 3 : min_free_kb/1024 > 9 ? 2 : 1)), "");

    printf("│ zswap enabled    : %-8s                          │\n", zswap_en);
    printf("│ MGLRU enabled    : %-8s                          │\n", mglru_en);
    printf("├────────────────────────────────────────────────────┤\n");

    printf("│ RAM  %5lu/%5lu MB  ", mem_used_kb/1024, mi.mem_total_kb/1024);
    bar(mem_used_kb, mi.mem_total_kb, 22);
    printf("  │\n");

    printf("│ SWAP %5lu/%5lu MB  ", swap_used_kb/1024, mi.swap_total_kb/1024);
    bar(swap_used_kb, mi.swap_total_kb, 22);
    printf("  │\n");

    printf("├────────────────────────────────────────────────────┤\n");
    printf("│ LRU SCAN ACTIVITY (/proc/vmstat)                   │\n");
    printf("│  pgscank  (kswapd background)  : %12lu       │\n", vs.pgscank);
    printf("│  pgscand  (direct reclaim!)    : %12lu       │\n", vs.pgscand);
    printf("│  pgsteal_kswapd                : %12lu       │\n", vs.pgsteal_kswapd);
    printf("│  pgsteal_direct                : %12lu       │\n", vs.pgsteal_direct);
    printf("├────────────────────────────────────────────────────┤\n");
    printf("│ SWAP I/O                                           │\n");
    printf("│  pswpin  (pages swapped in)    : %12lu       │\n", vs.pswpin);
    printf("│  pswpout (pages swapped out)   : %12lu       │\n", vs.pswpout);
    printf("│  SwapCached                    : %11lu MB   │\n", mi.swap_cached_kb/1024);
    printf("├────────────────────────────────────────────────────┤\n");

    if (vs.pgscand > 0 && vs.pgscank > 0) {
        double ratio = (double)vs.pgscand / (vs.pgscank + vs.pgscand) * 100.0;
        printf("│  ⚠  Direct reclaim = %.1f%% of scans (raise min_free!)  │\n", ratio);
    } else if (vs.pgscand > 0) {
        printf("│  ⚠  Direct reclaim active - raise vm.min_free_kbytes   │\n");
    } else {
        printf("│  ✓  No direct reclaim detected (healthy)                │\n");
    }

    uint64_t total_scan = vs.pgscank + vs.pgscand;
    if (total_scan > 0) {
        printf("│     Scan breakdown: kswapd %lu%%  direct %lu%%          │\n",
               vs.pgscank * 100 / total_scan,
               vs.pgscand * 100 / total_scan);
    }
    printf("└────────────────────────────────────────────────────┘\n\n");

    return EXIT_SUCCESS;
}
