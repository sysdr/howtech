/*
 * alloc_demo.c - ptmalloc fragmentation & malloc_trim() demonstration
 * Compile: gcc -Wall -Wextra -Werror -O2 -o alloc_demo alloc_demo.c
 *
 * Shows:
 *   1. RSS bloat from interleaved alloc/free (fragmentation)
 *   2. mallinfo2() revealing free blocks still held by ptmalloc
 *   3. malloc_trim() effect on top chunk
 *   4. mmap threshold vs brk behavior
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>

/* ── ANSI color helpers ────────────────────────────────────── */
#define COL_RESET  "\033[0m"
#define COL_CYAN   "\033[0;36m"
#define COL_YELLOW "\033[0;33m"
#define COL_GREEN  "\033[0;32m"
#define COL_RED    "\033[0;31m"
#define COL_DIM    "\033[2m"
#define COL_BOLD   "\033[1m"

/* ── Read RSS from /proc/self/status ───────────────────────── */
static long get_rss_kb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long rss = -1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            if (sscanf(line + 6, "%ld", &rss) != 1)
                rss = -1;
            break;
        }
    }
    fclose(f);
    return rss;
}

/* ── Read anonymous pages from /proc/self/smaps_rollup ─────── */
static long get_anon_kb(void)
{
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) return -1;
    char line[256];
    long anon = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Anonymous:", 10) == 0) {
            long val = 0;
            if (sscanf(line + 10, "%ld", &val) == 1)
                anon += val;
        }
    }
    fclose(f);
    return anon;
}

/* ── Read OOM score from /proc/self/oom_score ──────────────── */
static int get_oom_score(void)
{
    FILE *f = fopen("/proc/self/oom_score", "r");
    if (!f) return -1;
    int score = -1;
    if (fscanf(f, "%d", &score) != 1)
        score = -1;
    fclose(f);
    return score;
}

/* ── Print mallinfo2 summary ───────────────────────────────── */
static void print_mallinfo(const char *label)
{
    struct mallinfo2 mi = mallinfo2();
    printf(COL_CYAN "  [mallinfo2 @ %-26s]" COL_RESET "\n", label);
    printf(COL_DIM  "    arena      (non-mmap heap):  %12zu KB\n", mi.arena   / 1024);
    printf(         "    ordblks    (free chunks):     %12zu\n",   mi.ordblks);
    printf(         "    hblks      (mmap regions):    %12zu\n",   mi.hblks);
    printf(         "    hblkhd     (mmap bytes):      %12zu KB\n", mi.hblkhd  / 1024);
    printf(         "    uordblks   (in-use bytes):    %12zu KB\n", mi.uordblks/ 1024);
    printf(         "    fordblks   (free in arena):   %12zu KB" COL_RESET "\n", mi.fordblks/ 1024);
    long frag_pct = (mi.arena > 0)
        ? (long)(mi.fordblks * 100 / mi.arena)
        : 0;
    if (frag_pct > 30)
        printf(COL_RED    "    ↳ fragmentation: %ld%% (HIGH — RSS inflated)\n" COL_RESET, frag_pct);
    else
        printf(COL_GREEN  "    ↳ fragmentation: %ld%%\n" COL_RESET, frag_pct);
}

/* ── Print memory stats line ───────────────────────────────── */
static void print_mem(const char *label)
{
    long rss  = get_rss_kb();
    long anon = get_anon_kb();
    int  oom  = get_oom_score();
    printf(COL_YELLOW "  ► %-30s" COL_RESET
           " RSS=%6ld KB  Anon=%6ld KB  oom_score=%d\n",
           label, rss, anon, oom);
}

/* ── Phase 1: Demonstrate fragmentation ────────────────────── */
static void phase_fragmentation(void)
{
    printf(COL_BOLD "\n━━ Phase 1: Interleaved alloc/free → ptmalloc fragmentation ━━\n" COL_RESET);
    printf(COL_DIM  "  Pattern: alloc 1000 × 64KB chunks, free every other one\n"
                    "  Result:  arena holds freed blocks; RSS stays high\n\n" COL_RESET);

    enum { N_CHUNKS = 1000, CHUNK_SZ = 64 * 1024 };
    void *ptrs[N_CHUNKS];

    print_mem("before any allocation");

    /* Allocate all chunks */
    for (int i = 0; i < N_CHUNKS; i++) {
        ptrs[i] = malloc(CHUNK_SZ);
        if (!ptrs[i]) { perror("malloc"); exit(EXIT_FAILURE); }
        /* Touch pages so they're actually resident */
        memset(ptrs[i], (char)(i & 0xFF), CHUNK_SZ);
    }
    print_mem("after allocating 1000×64KB");

    /* Free every other chunk — creates holes in the arena */
    for (int i = 0; i < N_CHUNKS; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }
    print_mem("after freeing alternate chunks");
    print_mallinfo("alternate chunks freed");

    printf(COL_DIM  "\n  Note: fordblks > 0 means ptmalloc holds freed memory in bins.\n"
                    "  The OS still sees that RSS because pages weren't returned.\n\n" COL_RESET);

    /* Free remaining */
    for (int i = 1; i < N_CHUNKS; i += 2) {
        free(ptrs[i]);
        ptrs[i] = NULL;
    }
    print_mem("after freeing all chunks");
    print_mallinfo("all chunks freed");
}

/* ── Phase 2: malloc_trim() effect ─────────────────────────── */
static void phase_malloc_trim(void)
{
    printf(COL_BOLD "\n━━ Phase 2: malloc_trim() — returning top chunk to OS ━━\n" COL_RESET);
    printf(COL_DIM  "  malloc_trim(0) calls sbrk() to lower the program break.\n"
                    "  Only works on the wilderness (top) chunk, not interior holes.\n\n" COL_RESET);

    long rss_before = get_rss_kb();

    /* Trim */
    int trimmed = malloc_trim(0);
    long rss_after = get_rss_kb();

    printf("  malloc_trim(0) returned: %d (%s)\n", trimmed,
           trimmed ? "memory was returned to OS" : "nothing to return");
    printf("  RSS before trim: %ld KB\n", rss_before);
    printf("  RSS after  trim: %ld KB\n", rss_after);
    long saved = rss_before - rss_after;
    if (saved > 512)
        printf(COL_GREEN "  Reclaimed: %ld KB ✓\n" COL_RESET, saved);
    else
        printf(COL_YELLOW "  Reclaimed: %ld KB (interior fragmentation remains)\n" COL_RESET, saved > 0 ? saved : 0);

    print_mallinfo("after malloc_trim");
}

/* ── Phase 3: mmap threshold — large allocs bypass brk ─────── */
static void phase_mmap_threshold(void)
{
    printf(COL_BOLD "\n━━ Phase 3: mmap threshold — large allocs return immediately ━━\n" COL_RESET);
    printf(COL_DIM  "  Allocs > M_MMAP_THRESHOLD use mmap(MAP_ANON), not brk().\n"
                    "  On free(), mmap regions return to OS immediately (munmap).\n\n" COL_RESET);

    /* Default threshold is 128KB — let's demonstrate with a 512KB alloc */
    const size_t large_sz = 512 * 1024;
    print_mem("before large mmap alloc");

    void *p = malloc(large_sz);
    if (!p) { perror("malloc large"); return; }
    memset(p, 0xAB, large_sz);
    print_mem("after 512KB alloc (via mmap)");

    struct mallinfo2 mi_before = mallinfo2();
    printf(COL_DIM  "  hblks (mmap regions active): %zu\n" COL_RESET, mi_before.hblks);

    free(p);
    print_mem("after free — mmap region returned immediately");

    struct mallinfo2 mi_after = mallinfo2();
    printf(COL_DIM  "  hblks after free: %zu (munmap called)\n" COL_RESET, mi_after.hblks);

    printf(COL_DIM  "\n  Tuning tip: mallopt(M_MMAP_THRESHOLD, 256*1024) lowers the\n"
                    "  threshold so more allocations bypass the arena entirely.\n\n" COL_RESET);
}

/* ── Phase 4: OOM score + oom_score_adj ─────────────────────── */
static void phase_oom_score(void)
{
    printf(COL_BOLD "\n━━ Phase 4: OOM score analysis ━━\n" COL_RESET);

    long rss    = get_rss_kb();
    long mem_kb = 0;

    /* Read MemTotal from /proc/meminfo */
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line + 9, "%ld", &mem_kb);
                break;
            }
        }
        fclose(f);
    }

    int  oom_score = get_oom_score();
    int  oom_adj   = -1;
    FILE *fa = fopen("/proc/self/oom_score_adj", "r");
    if (fa) {
        if (fscanf(fa, "%d", &oom_adj) != 1)
            oom_adj = -1;
        fclose(fa);
    }

    printf("  PID:           %d\n", getpid());
    printf("  RSS:           %ld KB\n", rss);
    printf("  MemTotal:      %ld KB\n", mem_kb);
    printf("  oom_score:     %d  (range 0–1000+adj)\n", oom_score);
    printf("  oom_score_adj: %d  (/proc/self/oom_score_adj)\n", oom_adj);

    if (mem_kb > 0) {
        long base = (rss * 1000) / mem_kb;
        printf("  base score ≈   %ld  (RSS×1000 / MemTotal)\n", base);
    }

    printf(COL_DIM "\n  To protect a critical process:\n"
                   "    echo -500 > /proc/$(pgrep myservice)/oom_score_adj\n"
                   "  To make a process a preferred OOM victim:\n"
                   "    echo 500  > /proc/$(pgrep bloated-svc)/oom_score_adj\n\n" COL_RESET);
}

/* ── Phase 5: mallopt tuning recommendations ─────────────────── */
static void phase_tuning(void)
{
    printf(COL_BOLD "\n━━ Phase 5: Runtime ptmalloc tuning via mallopt() ━━\n" COL_RESET);

    /* Current values */
    printf("  Setting M_TRIM_THRESHOLD = 128KB (default ~512KB)\n");
    if (mallopt(M_TRIM_THRESHOLD, 128 * 1024) != 1)
        fprintf(stderr, "Warning: mallopt M_TRIM_THRESHOLD failed\n");
    else
        printf(COL_GREEN "  ✓ Heap will be trimmed when 128KB of top chunk is free\n" COL_RESET);

    printf("  Setting M_MMAP_THRESHOLD = 256KB (default 128KB)\n");
    if (mallopt(M_MMAP_THRESHOLD, 256 * 1024) != 1)
        fprintf(stderr, "Warning: mallopt M_MMAP_THRESHOLD failed\n");
    else
        printf(COL_GREEN "  ✓ Allocs > 256KB bypass arena — freed immediately\n" COL_RESET);

    printf(COL_DIM "\n  For jemalloc users (LD_PRELOAD):\n"
                   "    MALLOC_CONF=\"background_thread:true,dirty_decay_ms:500,narenas:4\"\n"
                   "  For tcmalloc users:\n"
                   "    TCMALLOC_RELEASE_RATE=10  (aggressively return to OS)\n\n" COL_RESET);
}

int main(void)
{
    printf(COL_BOLD COL_CYAN
           "\n╔══════════════════════════════════════════════════════════╗\n"
           "║  Memory Allocator Deep Dive — ptmalloc2 Behavior Demo   ║\n"
           "║  PID: %-8d                                          ║\n"
           "╚══════════════════════════════════════════════════════════╝\n"
           COL_RESET, getpid());

    printf(COL_DIM "  Tracking: RSS (/proc/self/status), Anonymous pages\n"
                   "  (/proc/self/smaps_rollup), mallinfo2(), oom_score\n" COL_RESET);

    phase_fragmentation();
    phase_malloc_trim();
    phase_mmap_threshold();
    phase_oom_score();
    phase_tuning();

    printf(COL_BOLD COL_GREEN
           "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
           "  Demo complete. Key takeaways:\n"
           COL_RESET COL_DIM
           "  1. ptmalloc free() ≠ memory returned to OS — check fordblks\n"
           "  2. malloc_trim() only reclaims the top chunk, not interior holes\n"
           "  3. Allocs above mmap_threshold use mmap — freed immediately\n"
           "  4. jemalloc with background_thread:true reduces RSS 30–60%%\n"
           "  5. Protect critical processes with oom_score_adj = -500\n"
           COL_RESET "\n");

    return 0;
}
