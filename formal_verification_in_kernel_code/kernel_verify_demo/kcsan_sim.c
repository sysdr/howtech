/*
 * kcsan_sim.c — Demonstrates data race detection (mirrors KCSAN behavior)
 *
 * KCSAN (Kernel Concurrency Sanitizer) uses a watchpoint mechanism:
 *   1. On each memory access, randomly arm a watchpoint on the address
 *   2. Insert a small delay (~1–100 µs) 
 *   3. If another CPU accesses the same address without synchronization
 *      during the delay window, report a race
 *
 * TSAN (Thread Sanitizer) uses happens-before tracking — same end result.
 * Both catch races that may never manifest as crashes on x86 (TSO) but 
 * will on ARM/POWER with weaker memory ordering.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ITERATIONS  100000

/* Shared state — simulates kernel per-CPU or global counters */
typedef struct {
    long    unprotected_counter;    /* RACE: no synchronization */
    atomic_long atomic_counter;     /* SAFE: atomic operations */
    pthread_mutex_t mutex;
    long    mutex_counter;          /* SAFE: mutex protected */

    /* Race stats */
    long    races_possible;         /* incremented without sync */
    long    races_atomic;           /* incremented atomically */
} shared_data_t;

static shared_data_t data;
static volatile int  start_flag = 0;

/* Worker thread — demonstrates both racy and safe patterns */
static void *worker(void *arg)
{
    long tid = (long)arg;
    struct timespec ts = { .tv_nsec = 100 };

    /* Wait for synchronous start (reduces startup jitter) */
    while (!start_flag) nanosleep(&ts, NULL);

    for (int i = 0; i < ITERATIONS; i++) {
        /* ── RACY: unprotected read-modify-write ─────────────────────── */
        /* Equivalent to: if (!rcu_dereference(p)) p->count++; without lock */
        data.unprotected_counter++;   /* read-modify-write, no barrier */

        /* ── SAFE: atomic operation ──────────────────────────────────── */
        atomic_fetch_add_explicit(&data.atomic_counter, 1,
                                   memory_order_relaxed);

        /* ── SAFE: mutex protected ───────────────────────────────────── */
        pthread_mutex_lock(&data.mutex);
        data.mutex_counter++;
        pthread_mutex_unlock(&data.mutex);
    }

    (void)tid;
    return NULL;
}

/* Simulate KCSAN watchpoint — randomly intercept accesses */
static int kcsan_watchpoint_check(const char *var_name, long expected_min,
                                   long actual, int n_threads, int n_iters)
{
    (void)expected_min; /* unused, kept for API consistency */
    long expected = (long)n_threads * n_iters;
    long delta    = actual - expected;
    double loss_pct = (double)(expected - actual) / expected * 100.0;

    printf("  %-30s: expected=%ld  actual=%ld  delta=%ld\n",
           var_name, expected, actual, delta);

    if (delta != 0) {
        printf("  \033[1;31mKCSAN would report: DATA RACE on %s\033[0m\n", var_name);
        printf("  \033[1;31mLost %ld updates (%.2f%%) — non-atomic RMW\033[0m\n",
               expected - actual, loss_pct > 0 ? loss_pct : 0.0);
        printf("  \033[0;33mOn x86: partially hidden by TSO (Total Store Order)\033[0m\n");
        printf("  \033[0;33mOn ARM: would corrupt more — weaker memory model\033[0m\n\n");
        return 1;
    }
    printf("  \033[0;32m✓ No lost updates — properly synchronized\033[0m\n\n");
    return 0;
}

int main(void)
{
    int n_threads = 4;
    pthread_t threads[4];

    printf("\033[1;36m=== KCSAN / Data Race Simulation ===\033[0m\n");
    printf("Threads: %d | Iterations per thread: %d | Total ops: %d\n\n",
           n_threads, ITERATIONS, n_threads * ITERATIONS);

    memset(&data, 0, sizeof(data));
    pthread_mutex_init(&data.mutex, NULL);
    atomic_store(&data.atomic_counter, 0);

    /* Spawn threads */
    for (long t = 0; t < n_threads; t++)
        pthread_create(&threads[t], NULL, worker, (void*)t);

    /* Synchronous start */
    atomic_thread_fence(memory_order_seq_cst);
    start_flag = 1;

    for (int t = 0; t < n_threads; t++)
        pthread_join(threads[t], NULL);

    printf("\033[1;33m--- Results: concurrent increment with %d threads ---\033[0m\n\n",
           n_threads);

    int races = 0;
    races += kcsan_watchpoint_check("unprotected_counter (RACY)",
                                     0, data.unprotected_counter,
                                     n_threads, ITERATIONS);
    races += kcsan_watchpoint_check("atomic_counter (SAFE)",
                                     0,
                                     atomic_load(&data.atomic_counter),
                                     n_threads, ITERATIONS);
    races += kcsan_watchpoint_check("mutex_counter (SAFE)",
                                     0, data.mutex_counter,
                                     n_threads, ITERATIONS);

    printf("\033[1;32m=== KCSAN Summary ===\033[0m\n");
    printf("Data races detected: %d/3 patterns\n", races);
    printf("KCSAN principle: arm watchpoint + delay → concurrent access = race\n");
    printf("KCSAN overhead: ~1-2x (watchpoint sampling, not every access)\n");
    printf("Production: KFENCE is preferred (lower overhead, OOB/UAF only)\n");

    pthread_mutex_destroy(&data.mutex);
    return (races >= 1) ? 0 : 1; /* We expect at least 1 race */
}
