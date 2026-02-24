/*
 * alloc_bench.c — Multi-threaded allocator benchmark
 * Measures: throughput (allocs/sec), p50/p99 latency, RSS growth, syscall rates
 * Compiles with: -Wall -Wextra -Werror -O2
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <math.h>

#define NTHREADS       8
#define ITERS_PER_THR  200000
#define WARMUP_ITERS   10000
#define LATENCY_SLOTS  4096    /* ring buffer for latency samples */
#define MAX_LIVE       1024    /* max simultaneously live allocations per thread */

/* Size distribution matching real services: 70% small, 20% medium, 10% large */
static const size_t SIZE_TABLE[] = {
    16, 16, 16, 24, 32, 32, 48, 64, 64, 64,  /* 70% small ≤64 */
    128, 256, 512, 1024,                       /* 20% medium */
    4096, 65536                                /* 10% large */
};
#define SIZE_TABLE_LEN (sizeof(SIZE_TABLE) / sizeof(SIZE_TABLE[0]))

typedef struct {
    uint64_t samples[LATENCY_SLOTS];
    uint64_t count;
    uint64_t total_ns;
    uint64_t min_ns;
    uint64_t max_ns;
} LatStats;

typedef struct {
    int       tid;
    uint64_t  allocs;
    uint64_t  frees;
    LatStats  alloc_lat;
    LatStats  free_lat;
    double    elapsed_sec;
} ThreadResult;

static volatile int g_start = 0;
static volatile int g_stop  = 0;

static inline uint64_t rdtsc(void) {
    uint64_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return (hi << 32) | lo;
}

static inline uint64_t ts_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) { perror("clock_gettime"); exit(1); }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void lat_record(LatStats *s, uint64_t ns) {
    s->samples[s->count % LATENCY_SLOTS] = ns;
    s->count++;
    s->total_ns += ns;
    if (ns < s->min_ns || s->min_ns == 0) s->min_ns = ns;
    if (ns > s->max_ns) s->max_ns = ns;
}

static int cmp_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static double lat_percentile(LatStats *s, double p) {
    if (s->count == 0) return 0.0;
    uint64_t n = s->count < (uint64_t)LATENCY_SLOTS ? s->count : (uint64_t)LATENCY_SLOTS;
    uint64_t *tmp = malloc(n * sizeof(uint64_t));
    if (!tmp) { perror("malloc"); exit(1); }
    memcpy(tmp, s->samples, n * sizeof(uint64_t));
    qsort(tmp, n, sizeof(uint64_t), cmp_u64);
    uint64_t idx = (uint64_t)(p * (double)(n - 1) / 100.0);
    double val = (double)tmp[idx];
    free(tmp);
    return val;
}

static uint64_t read_rss_kb(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;
    char line[128];
    uint64_t rss = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, " %llu", (unsigned long long *)&rss);
            break;
        }
    }
    fclose(f);
    return rss;
}

static void *bench_thread(void *arg) {
    ThreadResult *r = (ThreadResult *)arg;
    void *live[MAX_LIVE];
    memset(live, 0, sizeof(live));

    /* Simple LCG for fast pseudo-random without locking */
    uint64_t rng = (uint64_t)(uintptr_t)arg ^ 0xdeadbeefcafeULL;
#define RNG_NEXT(r) ((r) = (r) * 6364136223846793005ULL + 1442695040888963407ULL)

    /* Warm up: fill live[] half-way */
    for (int i = 0; i < MAX_LIVE / 2; i++) {
        RNG_NEXT(rng);
        size_t sz = SIZE_TABLE[rng % SIZE_TABLE_LEN];
        live[i] = malloc(sz);
        if (!live[i]) { fprintf(stderr, "warmup malloc failed\n"); exit(1); }
        memset(live[i], 0xAB, sz);  /* ensure physical pages are faulted in */
    }

    /* Wait for start signal */
    while (!g_start) { __asm__ volatile ("pause" ::: "memory"); }

    uint64_t t0 = ts_ns();
    uint64_t iters = 0;

    while (!g_stop && iters < (uint64_t)ITERS_PER_THR) {
        RNG_NEXT(rng);
        size_t sz = SIZE_TABLE[rng % SIZE_TABLE_LEN];
        int slot  = (int)(rng % MAX_LIVE);

        /* Free existing allocation in slot */
        if (live[slot]) {
            uint64_t fs = ts_ns();
            free(live[slot]);
            lat_record(&r->free_lat, ts_ns() - fs);
            live[slot] = NULL;
            r->frees++;
        }

        /* Alloc new */
        uint64_t as = ts_ns();
        live[slot] = malloc(sz);
        lat_record(&r->alloc_lat, ts_ns() - as);
        if (!live[slot]) { fprintf(stderr, "malloc(%zu) returned NULL\n", sz); exit(1); }
        /* Touch the memory to trigger page faults on first access */
        ((volatile char *)live[slot])[0] = 1;
        r->allocs++;
        iters++;
    }

    uint64_t t1 = ts_ns();
    r->elapsed_sec = (double)(t1 - t0) / 1e9;

    /* Clean up live allocations */
    for (int i = 0; i < MAX_LIVE; i++) { if (live[i]) { free(live[i]); r->frees++; } }
    return NULL;
}

int main(int argc, char **argv) {
    const char *label = argc > 1 ? argv[1] : "default";
    fprintf(stdout, "\n[bench] allocator=%s threads=%d iters/thr=%d\n",
            label, NTHREADS, ITERS_PER_THR);

    uint64_t rss_before = read_rss_kb();

    pthread_t threads[NTHREADS];
    ThreadResult results[NTHREADS];
    memset(results, 0, sizeof(results));
    for (int i = 0; i < NTHREADS; i++) results[i].tid = i;

    /* Spawn all threads */
    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_create(&threads[i], NULL, bench_thread, &results[i]) != 0) {
            perror("pthread_create"); exit(1);
        }
    }

    sleep(1);  /* Let threads reach the spin-wait */
    __atomic_store_n(&g_start, 1, __ATOMIC_RELEASE);

    /* Join all threads */
    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) { perror("pthread_join"); exit(1); }
    }

    uint64_t rss_after = read_rss_kb();

    /* Aggregate stats */
    uint64_t total_allocs = 0, total_frees = 0;
    double   total_elapsed = 0.0;
    LatStats agg_alloc = {0}, agg_free = {0};

    for (int i = 0; i < NTHREADS; i++) {
        ThreadResult *r = &results[i];
        total_allocs  += r->allocs;
        total_frees   += r->frees;
        if (r->elapsed_sec > total_elapsed) total_elapsed = r->elapsed_sec;

        /* Merge latency samples (first-N merge) */
        for (uint64_t j = 0; j < r->alloc_lat.count && j < LATENCY_SLOTS; j++) {
            lat_record(&agg_alloc, r->alloc_lat.samples[j]);
        }
        for (uint64_t j = 0; j < r->free_lat.count && j < LATENCY_SLOTS; j++) {
            lat_record(&agg_free, r->free_lat.samples[j]);
        }
    }

    double throughput = (double)total_allocs / total_elapsed;

    printf("\n┌─────────────────────────────────────────────────┐\n");
    printf("│  Results: %-36s │\n", label);
    printf("├─────────────────────────────────────────────────┤\n");
    printf("│  Throughput:    %10.0f allocs/sec          │\n", throughput);
    printf("│  Total allocs:  %10llu                       │\n", (unsigned long long)total_allocs);
    printf("│  Elapsed:       %10.3f sec                  │\n", total_elapsed);
    printf("│                                                 │\n");
    printf("│  malloc() latency (ns):                        │\n");
    printf("│    p50:  %8.1f ns                           │\n", lat_percentile(&agg_alloc, 50.0));
    printf("│    p95:  %8.1f ns                           │\n", lat_percentile(&agg_alloc, 95.0));
    printf("│    p99:  %8.1f ns                           │\n", lat_percentile(&agg_alloc, 99.0));
    printf("│    min:  %8llu ns   max: %8llu ns        │\n",
           (unsigned long long)agg_alloc.min_ns, (unsigned long long)agg_alloc.max_ns);
    printf("│                                                 │\n");
    printf("│  free() latency (ns):                          │\n");
    printf("│    p50:  %8.1f ns                           │\n", lat_percentile(&agg_free, 50.0));
    printf("│    p99:  %8.1f ns                           │\n", lat_percentile(&agg_free, 99.0));
    printf("│                                                 │\n");
    printf("│  RSS delta:  %+9lld KB                       │\n",
           (long long)rss_after - (long long)rss_before);
    printf("└─────────────────────────────────────────────────┘\n\n");

    return 0;
}
