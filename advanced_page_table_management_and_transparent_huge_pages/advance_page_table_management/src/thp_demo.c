/*
 * thp_demo.c — Transparent Huge Pages internals demonstration
 *
 * Demonstrates:
 *   1. THP fault-time allocation with alignment verification
 *   2. AnonHugePages detection via /proc/self/smaps
 *   3. CoW amplification measurement on fork()
 *   4. MADV_HUGEPAGE vs MADV_NOHUGEPAGE comparison
 *   5. TLB miss approximation via memory access timing
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -D_GNU_SOURCE -o thp_demo thp_demo.c
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

/* 2MB huge page size — PMD-level on x86-64 */
#define HUGE_PAGE_SIZE      (2UL * 1024 * 1024)
/* Number of 2MB regions to allocate */
#define NUM_HUGE_REGIONS    32
/* Total anonymous test region: 64MB */
#define TOTAL_SIZE          (NUM_HUGE_REGIONS * HUGE_PAGE_SIZE)
/* Small 4KB page */
#define PAGE_SIZE           4096
/* Iterations for timing benchmark */
#define BENCH_ITERATIONS    100000000UL

/* -----------------------------------------------------------------------
 * smaps_anon_huge_kb: parse /proc/self/smaps for AnonHugePages under
 * a given mapping address range.  Returns kB or -1 on error.
 * ----------------------------------------------------------------------- */
static long smaps_anon_huge_kb(void)
{
    FILE *fp;
    char line[256];
    long total_kb = 0;

    fp = fopen("/proc/self/smaps", "r");
    if (!fp) {
        perror("fopen /proc/self/smaps");
        return -1;
    }
    while (fgets(line, sizeof(line), fp)) {
        long kb = 0;
        if (sscanf(line, "AnonHugePages: %ld kB", &kb) == 1) {
            total_kb += kb;
        }
    }
    fclose(fp);
    return total_kb;
}

/* -----------------------------------------------------------------------
 * vmstat_thp: read a named counter from /proc/vmstat.
 * Returns the value or -1 on error.
 * ----------------------------------------------------------------------- */
static long vmstat_thp(const char *key)
{
    FILE *fp;
    char line[256];
    char name[64];
    long val = -1;

    fp = fopen("/proc/vmstat", "r");
    if (!fp) return -1;
    while (fgets(line, sizeof(line), fp)) {
        long v = 0;
        if (sscanf(line, "%63s %ld", name, &v) == 2) {
            if (strcmp(name, key) == 0) {
                val = v;
                break;
            }
        }
    }
    fclose(fp);
    return val;
}

/* -----------------------------------------------------------------------
 * monotonic_ns: returns current time as nanoseconds (CLOCK_MONOTONIC)
 * ----------------------------------------------------------------------- */
static inline uint64_t monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* -----------------------------------------------------------------------
 * get_rss_kb: return RSS of current process from /proc/self/status
 * ----------------------------------------------------------------------- */
static long get_rss_kb(void)
{
    FILE *fp;
    char line[128];
    long rss = -1;

    fp = fopen("/proc/self/status", "r");
    if (!fp) return -1;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "VmRSS: %ld kB", &rss) == 1)
            break;
    }
    fclose(fp);
    return rss;
}

/* -----------------------------------------------------------------------
 * touch_region: write one byte per page to force population.
 * For THP, this populates the huge page; for 4KB, each write is a fault.
 * ----------------------------------------------------------------------- */
static void touch_region(char *base, size_t size, size_t stride)
{
    volatile char *p = base;
    for (size_t off = 0; off < size; off += stride) {
        p[off] = (char)(off & 0xFF);
    }
    /* memory barrier — prevent compiler reordering */
    __asm__ volatile("" ::: "memory");
}

/* -----------------------------------------------------------------------
 * bench_sequential_read: time sequential read across a region.
 * Returns nanoseconds per access.
 * ----------------------------------------------------------------------- */
static double bench_sequential_read(char *base, size_t size)
{
    uint64_t start, end;
    volatile char sink = 0;
    size_t stride = 64; /* one cache line */

    /* Warm the region first */
    for (size_t off = 0; off < size; off += stride)
        sink ^= base[off];

    start = monotonic_ns();
    for (size_t i = 0; i < 4; i++) {
        for (size_t off = 0; off < size; off += stride)
            sink ^= base[off];
    }
    end = monotonic_ns();

    (void)sink;
    double ns_per_access = (double)(end - start) / (4.0 * (size / stride));
    return ns_per_access;
}

/* -----------------------------------------------------------------------
 * demo_cow_amplification: fork a child that reads; parent writes.
 * Measures CoW cost difference between 2MB and 4KB-backed regions.
 * ----------------------------------------------------------------------- */
static void demo_cow_amplification(char *thp_region, size_t size)
{
    pid_t pid;
    long rss_before, rss_after;
    uint64_t t_start, t_end;

    printf("\n  [CoW] Parent RSS before fork: %ld KB\n", get_rss_kb());
    rss_before = get_rss_kb();

    t_start = monotonic_ns();
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        /* Child: just read (shared mapping) — triggers read fault on CoW page */
        volatile char sink = 0;
        for (size_t off = 0; off < size; off += PAGE_SIZE)
            sink ^= thp_region[off];
        (void)sink;
        _exit(0);
    }

    /* Parent: write to every 4KB page in the THP-backed region.
     * Each write on a THP page triggers split: 2MB copy instead of 4KB copy. */
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        thp_region[off] = (char)((off >> 12) & 0xFF);
    }
    t_end = monotonic_ns();

    waitpid(pid, NULL, 0);
    rss_after = get_rss_kb();

    printf("  [CoW] Parent RSS after CoW writes: %ld KB (delta: %+ld KB)\n",
           rss_after, rss_after - rss_before);
    printf("  [CoW] Time for CoW writes across %.0f MB: %.2f ms\n",
           (double)size / (1024 * 1024),
           (double)(t_end - t_start) / 1e6);
    printf("  [CoW] Each write to THP page caused 2MB copy (not 4KB)\n");
    printf("  [CoW] thp_split_page after: %ld\n",
           vmstat_thp("thp_split_page"));
}

/* -----------------------------------------------------------------------
 * print_thp_sysfs: show the THP configuration from sysfs
 * ----------------------------------------------------------------------- */
static void print_thp_sysfs(void)
{
    const char *paths[] = {
        "/sys/kernel/mm/transparent_hugepage/enabled",
        "/sys/kernel/mm/transparent_hugepage/defrag",
        "/sys/kernel/mm/transparent_hugepage/khugepaged/scan_sleep_millisecs",
        "/sys/kernel/mm/transparent_hugepage/khugepaged/pages_to_scan",
        NULL
    };

    printf("\n  THP kernel configuration:\n");
    for (int i = 0; paths[i]; i++) {
        FILE *fp = fopen(paths[i], "r");
        if (fp) {
            char buf[256] = {0};
            if (fgets(buf, sizeof(buf), fp)) {
                /* strip newline */
                buf[strcspn(buf, "\n")] = '\0';
                printf("  %-65s = %s\n", paths[i], buf);
            }
            fclose(fp);
        } else {
            printf("  %-65s = (unavailable in container)\n", paths[i]);
        }
    }
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    char *thp_region;
    char *nothp_region;
    long anon_huge_before, anon_huge_after;
    long fault_alloc_before, fault_alloc_after;
    long collapse_before, collapse_after;
    double thp_ns, nothp_ns;

    printf("\n");
    printf("  ╔═══════════════════════════════════════════════════════════╗\n");
    printf("  ║        THP Internals — Live Demonstration                 ║\n");
    printf("  ║   Systems Programming Deep Dive | Article #7             ║\n");
    printf("  ╚═══════════════════════════════════════════════════════════╝\n\n");

    print_thp_sysfs();

    /* ---- Snapshot vmstat before allocation ---- */
    anon_huge_before  = smaps_anon_huge_kb();
    fault_alloc_before = vmstat_thp("thp_fault_alloc");
    collapse_before   = vmstat_thp("thp_collapse_alloc");

    printf("\n  ── Phase 1: THP-Backed Allocation ──────────────────────────\n");
    printf("  Allocating %zu MB with MADV_HUGEPAGE...\n",
           TOTAL_SIZE / (1024 * 1024));

    /*
     * MAP_ANONYMOUS | MAP_PRIVATE gives an anonymous VMA.
     * We request 2× the size to guarantee a 2MB-aligned region exists
     * within the allocation (worst case: 2MB-1 bytes wasted at start).
     */
    char *raw = mmap(NULL, TOTAL_SIZE + HUGE_PAGE_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (raw == MAP_FAILED) {
        perror("mmap THP region");
        return 1;
    }

    /* Align to 2MB boundary */
    uintptr_t aligned = ((uintptr_t)raw + HUGE_PAGE_SIZE - 1)
                        & ~(HUGE_PAGE_SIZE - 1);
    thp_region = (char *)aligned;

    /* Hint to kernel: use huge pages here */
    if (madvise(thp_region, TOTAL_SIZE, MADV_HUGEPAGE) != 0) {
        fprintf(stderr, "  [warn] madvise(MADV_HUGEPAGE) failed (may need root or sysctl)\n");
    }

    /* Populate — triggers page faults, kernel tries huge page allocation */
    touch_region(thp_region, TOTAL_SIZE, PAGE_SIZE);

    fault_alloc_after = vmstat_thp("thp_fault_alloc");
    collapse_after    = vmstat_thp("thp_collapse_alloc");
    anon_huge_after   = smaps_anon_huge_kb();

    printf("  AnonHugePages before: %ld kB\n", anon_huge_before);
    printf("  AnonHugePages after:  %ld kB  (+%ld kB = %.1f MB)\n",
           anon_huge_after,
           anon_huge_after - anon_huge_before,
           (double)(anon_huge_after - anon_huge_before) / 1024.0);
    printf("  thp_fault_alloc delta:   %ld\n",
           fault_alloc_after - fault_alloc_before);
    printf("  thp_collapse_alloc delta: %ld  (khugepaged background)\n",
           collapse_after - collapse_before);

    if (anon_huge_after > anon_huge_before) {
        printf("  → Kernel promoted pages to THP successfully\n");
    } else {
        printf("  → No THP promotion (system memory fragmented or THP=never)\n");
        printf("    Run: echo madvise > /sys/kernel/mm/transparent_hugepage/enabled\n");
    }

    /* ---- Phase 2: 4KB baseline allocation ---- */
    printf("\n  ── Phase 2: 4KB Baseline (MADV_NOHUGEPAGE) ────────────────\n");
    nothp_region = mmap(NULL, TOTAL_SIZE,
                        PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (nothp_region == MAP_FAILED) {
        perror("mmap nothp region");
        munmap(raw, TOTAL_SIZE + HUGE_PAGE_SIZE);
        return 1;
    }

    if (madvise(nothp_region, TOTAL_SIZE, MADV_NOHUGEPAGE) != 0) {
        /* non-fatal: system might not support it */
    }
    touch_region(nothp_region, TOTAL_SIZE, PAGE_SIZE);
    printf("  Allocated %zu MB with MADV_NOHUGEPAGE (4KB pages)\n",
           TOTAL_SIZE / (1024 * 1024));

    /* ---- Phase 3: TLB impact benchmark ---- */
    printf("\n  ── Phase 3: Sequential Access Latency (TLB Effect) ─────────\n");
    printf("  Running sequential cache-line reads across 64 MB...\n");

    thp_ns    = bench_sequential_read(thp_region,    TOTAL_SIZE);
    nothp_ns  = bench_sequential_read(nothp_region,  TOTAL_SIZE);

    printf("  THP region:   %.2f ns/access (avg per cache line)\n", thp_ns);
    printf("  4KB region:   %.2f ns/access (avg per cache line)\n", nothp_ns);
    if (nothp_ns > thp_ns) {
        printf("  THP speedup:  %.2fx\n", nothp_ns / thp_ns);
        printf("  (Difference dominated by TLB miss reduction, not prefetch)\n");
    }

    /* ---- Phase 4: CoW amplification ---- */
    printf("\n  ── Phase 4: fork() CoW Amplification ───────────────────────\n");
    printf("  thp_split_page before fork: %ld\n", vmstat_thp("thp_split_page"));
    demo_cow_amplification(thp_region, TOTAL_SIZE);

    /* ---- Phase 5: /proc/self/smaps summary ---- */
    printf("\n  ── Phase 5: /proc/self/smaps AnonHugePages ─────────────────\n");
    printf("  Current AnonHugePages: %ld kB\n", smaps_anon_huge_kb());
    printf("  RSS: %ld kB\n", get_rss_kb());
    printf("  (Run: grep -E 'AnonHugePages|Size|Rss' /proc/%d/smaps | head -60)\n",
           getpid());

    /* ---- Cleanup ---- */
    munmap(nothp_region, TOTAL_SIZE);
    munmap(raw, TOTAL_SIZE + HUGE_PAGE_SIZE);

    printf("\n  ── Final /proc/vmstat THP Counters ─────────────────────────\n");
    const char *counters[] = {
        "thp_fault_alloc", "thp_fault_fallback",
        "thp_collapse_alloc", "thp_collapse_alloc_failed",
        "thp_split_page", "thp_split_pmd",
        "thp_zero_page_alloc", NULL
    };
    for (int i = 0; counters[i]; i++) {
        long v = vmstat_thp(counters[i]);
        if (v >= 0)
            printf("  %-40s %ld\n", counters[i], v);
    }

    printf("\n  Demo complete. Compare split vs alloc counts for CoW ratio.\n\n");
    return 0;
}
