/*
 * counter_bench.c
 * Demonstrates the performance difference between:
 *   1. A shared global atomic counter (cache line bouncing)
 *   2. Per-thread (per-CPU analog) local counters (MESI Exclusive)
 *
 * Build: gcc -Wall -Wextra -Werror -O2 -pthread -o counter_bench counter_bench.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>

/* ── Config ─────────────────────────────────────────── */
#define MAX_THREADS  16
#define ITERATIONS   10000000ULL  /* 10M per thread */
#define CACHE_LINE   64

/* ── ANSI colours (minimal) ─────────────────────────── */
#define RED    "\033[0;31m"
#define GRN    "\033[0;32m"
#define YLW    "\033[0;33m"
#define BLU    "\033[0;34m"
#define CYN    "\033[0;36m"
#define WHT    "\033[1;37m"
#define DIM    "\033[2m"
#define RST    "\033[0m"

/* ── Shared global atomic (the bad case) ────────────── */
/* Padded to its own cache line to avoid false sharing with other vars */
typedef struct {
    atomic_long value;
    char _pad[CACHE_LINE - sizeof(atomic_long)];
} __attribute__((aligned(CACHE_LINE))) global_counter_t;

static global_counter_t g_counter;

/* ── Per-thread counter (the good case) ─────────────── */
/* Each thread's counter gets its own cache line */
typedef struct {
    long value;
    char _pad[CACHE_LINE - sizeof(long)];
} __attribute__((aligned(CACHE_LINE))) percpu_counter_t;

static percpu_counter_t percpu[MAX_THREADS];

/* ── Thread args ─────────────────────────────────────── */
typedef struct {
    int    tid;
    int    num_threads;
    long   result;          /* filled in after per-CPU run */
    double elapsed_ns;
} thread_arg_t;

/* ── Timing helper ───────────────────────────────────── */
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static double now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* ── Barrier (simple spin barrier) ──────────────────── */
typedef struct {
    atomic_int count;
    atomic_int gen;
    int        total;
} barrier_t;

static barrier_t g_barrier;

static void barrier_init(barrier_t *b, int n)
{
    atomic_store(&b->count, 0);
    atomic_store(&b->gen,   0);
    b->total = n;
}

static void barrier_wait(barrier_t *b)
{
    int gen = atomic_load(&b->gen);
    if (atomic_fetch_add(&b->count, 1) == b->total - 1) {
        atomic_store(&b->count, 0);
        atomic_fetch_add(&b->gen, 1);
    } else {
        while (atomic_load(&b->gen) == gen)
            ;   /* spin */
    }
}

/* ── Worker: global atomic ───────────────────────────── */
static void *worker_global(void *arg)
{
    thread_arg_t *a = (thread_arg_t *)arg;
    barrier_wait(&g_barrier);

    double t0 = now_ns();
    for (uint64_t i = 0; i < ITERATIONS; i++)
        atomic_fetch_add_explicit(&g_counter.value, 1, memory_order_relaxed);

    a->elapsed_ns = now_ns() - t0;
    return NULL;
}

/* ── Worker: per-CPU (thread-local cache line) ────────── */
static void *worker_percpu(void *arg)
{
    thread_arg_t *a = (thread_arg_t *)arg;
    barrier_wait(&g_barrier);

    double t0 = now_ns();
    for (uint64_t i = 0; i < ITERATIONS; i++)
        percpu[a->tid].value++;          /* hits Exclusive cache line */

    a->elapsed_ns  = now_ns() - t0;
    a->result      = percpu[a->tid].value;
    return NULL;
}

/* ── Pretty-print a number with commas ───────────────── */
static void print_commas(long n, char *buf, size_t sz)
{
    char tmp[32];
    int  len   = snprintf(tmp, sizeof(tmp), "%ld", n < 0 ? -n : n);
    int  out   = 0;
    int  comma = len % 3;
    if (n < 0 && sz > 1) { buf[out++] = '-'; sz--; }
    for (int i = 0; i < len && out < (int)sz - 1; i++) {
        if (i && i % 3 == comma) buf[out++] = ',';
        buf[out++] = tmp[i];
    }
    buf[out] = '\0';
}

/* ── Run a benchmark (both modes) ────────────────────── */
static void run_benchmark(int nthreads)
{
    pthread_t   tids[MAX_THREADS];
    thread_arg_t args[MAX_THREADS];
    double      wall_start, wall_end;
    double      total_global_ns = 0, total_percpu_ns = 0;
    char        buf1[32], buf2[32], buf3[32];

    /* ── GLOBAL ATOMIC ── */
    atomic_store(&g_counter.value, 0);
    barrier_init(&g_barrier, nthreads);

    for (int i = 0; i < nthreads; i++) {
        args[i].tid = i;
        args[i].num_threads = nthreads;
        if (pthread_create(&tids[i], NULL, worker_global, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    wall_start = now_ns();
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(tids[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        total_global_ns += args[i].elapsed_ns;
    }
    wall_end = now_ns();
    double wall_global = wall_end - wall_start;
    long   global_val  = atomic_load(&g_counter.value);

    /* ── PER-CPU ── */
    memset(percpu, 0, sizeof(percpu));
    barrier_init(&g_barrier, nthreads);

    for (int i = 0; i < nthreads; i++) {
        args[i].tid = i;
        args[i].num_threads = nthreads;
        args[i].result = 0;
        if (pthread_create(&tids[i], NULL, worker_percpu, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    wall_start = now_ns();
    for (int i = 0; i < nthreads; i++) {
        if (pthread_join(tids[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
        total_percpu_ns += args[i].elapsed_ns;
    }
    wall_end = now_ns();
    double wall_percpu = wall_end - wall_start;

    /* Aggregate per-CPU total */
    long percpu_total = 0;
    for (int i = 0; i < nthreads; i++)
        percpu_total += percpu[i].value;

    /* ── Results ── */
    double ops_global = (double)(ITERATIONS * (uint64_t)nthreads) / (wall_global / 1e9);
    double ops_percpu = (double)(ITERATIONS * (uint64_t)nthreads) / (wall_percpu / 1e9);
    double speedup    = ops_percpu / ops_global;

    print_commas((long)ops_global, buf1, sizeof(buf1));
    print_commas((long)ops_percpu, buf2, sizeof(buf2));
    snprintf(buf3, sizeof(buf3), "%.1fx", speedup);

    printf("  │ %s%2d threads%s │ "
           "atomic: %s%-14s%s ops/s  "
           "percpu: %s%-14s%s ops/s  "
           "speedup: %s%s%s\n",
           CYN, nthreads, RST,
           RED, buf1, RST,
           GRN, buf2, RST,
           speedup > 2.0 ? GRN : YLW, buf3, RST);

    (void)global_val;   /* suppress unused warning */
    (void)percpu_total;
}

/* ── Pretty header box ───────────────────────────────── */
static void print_header(void)
{
    printf("\n");
    printf("%s╔══════════════════════════════════════════════════════════════════════════╗%s\n", BLU, RST);
    printf("%s║%s  %sPer-CPU Counter Benchmark%s  —  global atomic vs. per-thread cache lines  %s║%s\n", BLU, RST, WHT, RST, BLU, RST);
    printf("%s╚══════════════════════════════════════════════════════════════════════════╝%s\n", BLU, RST);
    printf("  %s%llu iterations/thread%s  │  "
           "%sred%s = shared atomic  │  "
           "%sgreen%s = per-CPU local\n\n",
           DIM, ITERATIONS, RST, RED, RST, GRN, RST);
}

/* ── Explain MESI ─────────────────────────────────────── */
static void explain_mesi(int ncpus)
{
    printf("\n%s── MESI Analysis ──────────────────────────────────────────────────%s\n", BLU, RST);
    printf("  %sGlobal atomic:%s  Every write on CPU-N forces %d other CPUs to\n", RED, RST, ncpus - 1);
    printf("    invalidate their copy (I-state). Next access ⟶ bus fetch (~200-300ns).\n");
    printf("    Under load this becomes a %scache-line ping-pong%s — bus saturation.\n\n", YLW, RST);
    printf("  %sPer-CPU local:%s  Each CPU holds its counter in %sExclusive%s (E) state.\n", GRN, RST, GRN, RST);
    printf("    Writes stay local to L1. Zero cross-CPU invalidations.\n");
    printf("    Aggregation reads all per-CPU values once — %sapproximate%s but fast.\n", YLW, RST);
    printf("\n  %sKernel uses this in:%s SLUB freelist, percpu_counter (ext4/btrfs),\n", CYN, RST);
    printf("    network Rx/Tx stats, RCU callbacks, sched runqueues.\n\n");
}

int main(void)
{
    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus <= 0) ncpus = 4;
    if (ncpus > MAX_THREADS) ncpus = MAX_THREADS;

    print_header();
    printf("  System CPUs online: %s%ld%s\n\n", CYN, ncpus, RST);
    printf("  %s│ Threads  │ Global Atomic (ops/s)     │ Per-CPU (ops/s)           │ Speedup%s\n", DIM, RST);
    printf("  %s├──────────┼───────────────────────────┼───────────────────────────┤%s\n", DIM, RST);

    int thread_counts[] = {1, 2, 4, 8, 16};
    for (int i = 0; i < (int)(sizeof(thread_counts)/sizeof(thread_counts[0])); i++) {
        if (thread_counts[i] > (int)ncpus * 2) break;
        run_benchmark(thread_counts[i]);
    }

    explain_mesi((int)ncpus);
    return 0;
}
