// SCHED_BATCH demo benchmark:
// - 4 "interactive" threads (SCHED_OTHER) that nanosleep(1ms) and measure wake latency
// - 4 "batch" threads (SCHED_BATCH) that run CPU-bound work
// This is intended to be runnable as an unprivileged user on Linux.

#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static inline uint64_t nsec_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int set_self_policy(int policy) {
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = 0;
  if (sched_setscheduler(0, policy, &sp) != 0) return -errno;
  return 0;
}

static uint64_t fnv1a_rounds(uint64_t rounds) {
  // 64-bit FNV-1a (cheap CPU work)
  uint64_t hash = 1469598103934665603ull;
  const uint64_t prime = 1099511628211ull;
  for (uint64_t i = 0; i < rounds; i++) {
    hash ^= (i + (hash >> 32));
    hash *= prime;
  }
  return hash;
}

enum { N_INTERACTIVE = 4, N_BATCH = 4, ITERS = 500 };

struct interactive_result {
  uint64_t lat_ns[ITERS];
};

struct thread_arg {
  int idx;
  bool is_batch;
  struct interactive_result *ires;
};

static void *interactive_thread(void *p) {
  struct thread_arg *a = (struct thread_arg *)p;
  (void)a;

  // Ensure SCHED_OTHER (default), ignore errors (some containers restrict).
  (void)set_self_policy(SCHED_OTHER);

  const uint64_t sleep_ns = 1000000ull;  // 1ms
  struct timespec req;
  req.tv_sec = 0;
  req.tv_nsec = (long)sleep_ns;

  uint64_t target = nsec_now();
  for (int i = 0; i < ITERS; i++) {
    target += sleep_ns;
    // Use absolute-ish target to reduce drift; still nanosleep is relative.
    nanosleep(&req, NULL);
    uint64_t now = nsec_now();
    uint64_t lat = (now > target) ? (now - target) : 0;
    a->ires->lat_ns[i] = lat;
  }
  return NULL;
}

static void *batch_thread(void *p) {
  struct thread_arg *a = (struct thread_arg *)p;
  (void)a;

  int rc = set_self_policy(SCHED_BATCH);
  if (rc != 0) {
    fprintf(stderr, "WARN: failed to set SCHED_BATCH for batch thread (%d): %s\n",
            -rc, strerror(-rc));
  }

  // Run some CPU work. Value chosen to be noticeable but not long-running.
  volatile uint64_t sink = 0;
  for (int i = 0; i < ITERS; i++) {
    sink ^= fnv1a_rounds(5000000ull);
  }
  (void)sink;
  return NULL;
}

static int cmp_u64(const void *a, const void *b) {
  const uint64_t ua = *(const uint64_t *)a;
  const uint64_t ub = *(const uint64_t *)b;
  return (ua > ub) - (ua < ub);
}

static uint64_t percentile(uint64_t *arr, size_t n, double p) {
  if (n == 0) return 0;
  if (p < 0) p = 0;
  if (p > 1) p = 1;
  double idx = p * (double)(n - 1);
  size_t i = (size_t)(idx + 0.5);  // nearest-rank-ish
  if (i >= n) i = n - 1;
  return arr[i];
}

int main(void) {
  printf("sched_bench: %d interactive + %d batch threads, %d iters\n",
         N_INTERACTIVE, N_BATCH, ITERS);

  pthread_t threads[N_INTERACTIVE + N_BATCH];
  struct interactive_result results[N_INTERACTIVE];
  struct thread_arg args[N_INTERACTIVE + N_BATCH];

  // Start interactive threads first.
  for (int i = 0; i < N_INTERACTIVE; i++) {
    args[i] = (struct thread_arg){.idx = i, .is_batch = false, .ires = &results[i]};
    int rc = pthread_create(&threads[i], NULL, interactive_thread, &args[i]);
    if (rc != 0) {
      fprintf(stderr, "ERROR: pthread_create interactive[%d]: %s\n", i, strerror(rc));
      return 1;
    }
  }

  // Then batch threads.
  for (int i = 0; i < N_BATCH; i++) {
    int ti = N_INTERACTIVE + i;
    args[ti] = (struct thread_arg){.idx = i, .is_batch = true, .ires = NULL};
    int rc = pthread_create(&threads[ti], NULL, batch_thread, &args[ti]);
    if (rc != 0) {
      fprintf(stderr, "ERROR: pthread_create batch[%d]: %s\n", i, strerror(rc));
      return 1;
    }
  }

  for (int i = 0; i < N_INTERACTIVE + N_BATCH; i++) {
    pthread_join(threads[i], NULL);
  }

  // Flatten interactive latencies.
  uint64_t all[ (size_t)N_INTERACTIVE * (size_t)ITERS ];
  size_t n = 0;
  for (int t = 0; t < N_INTERACTIVE; t++) {
    for (int i = 0; i < ITERS; i++) all[n++] = results[t].lat_ns[i];
  }

  qsort(all, n, sizeof(all[0]), cmp_u64);

  uint64_t p50 = percentile(all, n, 0.50);
  uint64_t p95 = percentile(all, n, 0.95);
  uint64_t p99 = percentile(all, n, 0.99);
  uint64_t max = all[n - 1];

  printf("\nInteractive wake latency (ns):\n");
  printf("  p50=%" PRIu64 "  p95=%" PRIu64 "  p99=%" PRIu64 "  max=%" PRIu64 "\n",
         p50, p95, p99, max);
  printf("\nNote: batch threads attempt SCHED_BATCH; inspect with chrt/ /proc to verify.\n");
  return 0;
}

