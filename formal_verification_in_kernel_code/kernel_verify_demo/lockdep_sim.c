/*
 * lockdep_sim.c — Simulates Linux lockdep's lock dependency tracking
 *
 * lockdep builds a directed graph of lock acquisition orders.
 * It detects potential AB-BA deadlocks BEFORE they occur by checking
 * if the reverse ordering has been observed in any previous execution.
 *
 * Real lockdep: kernel/locking/lockdep.c ~13,000 lines
 * This demonstrates the core algorithm: cycle detection in acquisition DAG
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define MAX_LOCKS    32
#define MAX_EDGES    256
#define MAX_HELD     16

/* Lock class — equivalent to struct lock_class in kernel */
typedef struct {
    const char *name;
    const char *file;
    int         line;
    int         id;
} lock_class_t;

/* Dependency edge in the lock graph */
typedef struct {
    int from_class;  /* held when acquiring to_class */
    int to_class;
    const char *held_at_file;
    int held_at_line;
} lock_edge_t;

/* Per-thread held lock stack */
typedef struct {
    int class_ids[MAX_HELD];
    int depth;
} held_locks_t;

static lock_class_t  classes[MAX_LOCKS];
static int           nclasses = 0;
static lock_edge_t   edges[MAX_EDGES];
static int           nedges = 0;
static __thread held_locks_t held;

static int warnings = 0;
static int locks_checked = 0;

/* ── Register a lock class ──────────────────────────────────────────────── */
static int lockdep_register_class(const char *name, const char *file, int line)
{
    for (int i = 0; i < nclasses; i++)
        if (strcmp(classes[i].name, name) == 0) return i;

    if (nclasses >= MAX_LOCKS) return -1;
    classes[nclasses] = (lock_class_t){ name, file, line, nclasses };
    return nclasses++;
}

/* ── Cycle detection (DFS) ─────────────────────────────────────────────── */
static int visited[MAX_LOCKS];

static int dfs_cycle(int from, int target)
{
    if (from == target) return 1;
    if (visited[from]) return 0;
    visited[from] = 1;
    for (int i = 0; i < nedges; i++) {
        if (edges[i].from_class == from) {
            if (dfs_cycle(edges[i].to_class, target)) return 1;
        }
    }
    return 0;
}

/* ── Add dependency, check for cycles ──────────────────────────────────── */
static int lockdep_add_dependency(int from_class, int to_class,
                                   const char *file, int line)
{
    /* Check if this exact edge already exists */
    for (int i = 0; i < nedges; i++)
        if (edges[i].from_class == from_class &&
            edges[i].to_class   == to_class)
            return 0; /* known safe edge */

    /* Would adding from→to create a cycle? */
    memset(visited, 0, sizeof(visited));
    if (dfs_cycle(to_class, from_class)) {
        /* DEADLOCK WARNING */
        warnings++;
        printf("\n\033[1;31m=========================================================\033[0m\n");
        printf("\033[1;31mWARNING: possible circular locking dependency detected\033[0m\n");
        printf("Current task: some_kernel_thread/42\n\n");
        printf("   \033[1;33m%s\033[0m -> \033[1;33m%s\033[0m\n",
               classes[from_class].name, classes[to_class].name);
        printf("   But also known: \033[1;33m%s\033[0m -> \033[1;33m%s\033[0m\n",
               classes[to_class].name, classes[from_class].name);
        printf("\nLock chains:\n");
        printf("  Chain 1: CPU 0 holds %-12s -> acquiring %s\n",
               classes[from_class].name, classes[to_class].name);
        printf("  Chain 2: CPU 1 holds %-12s -> acquiring %s\n",
               classes[to_class].name, classes[from_class].name);
        printf("\nThis is an AB-BA deadlock — would deadlock if both CPUs\n");
        printf("reach these code paths concurrently.\n");
        printf("\033[1;31m=========================================================\033[0m\n\n");
        return 1; /* cycle detected, do NOT add edge */
    }

    if (nedges < MAX_EDGES) {
        edges[nedges++] = (lock_edge_t){ from_class, to_class, file, line };
    }
    return 0;
}

/* ── Simulated lock/unlock ──────────────────────────────────────────────── */
static void sim_lock(int class_id, const char *name, const char *file, int line)
{
    (void)name; /* unused, kept for API consistency */
    locks_checked++;

    /* For each lock currently held, record that held→this is a dependency */
    for (int i = 0; i < held.depth; i++) {
        lockdep_add_dependency(held.class_ids[i], class_id, file, line);
    }

    if (held.depth < MAX_HELD)
        held.class_ids[held.depth++] = class_id;
}

static void sim_unlock(int class_id)
{
    /* Pop from held stack (simplified — doesn't handle out-of-order) */
    for (int i = held.depth - 1; i >= 0; i--) {
        if (held.class_ids[i] == class_id) {
            memmove(&held.class_ids[i], &held.class_ids[i+1],
                    (held.depth - i - 1) * sizeof(int));
            held.depth--;
            return;
        }
    }
}

#define LOCK(cls)   sim_lock(cls, classes[cls].name, __FILE__, __LINE__)
#define UNLOCK(cls) sim_unlock(cls)

int main(void)
{
    printf("\033[1;36m=== lockdep Simulation — Lock Dependency Graph Analysis ===\033[0m\n\n");

    /* Register lock classes — mirrors how kernel initializes lock_class_key */
    int LA = lockdep_register_class("rtnl_lock",     __FILE__, __LINE__);
    int LB = lockdep_register_class("socket_lock",   __FILE__, __LINE__);
    int LC = lockdep_register_class("sk_receive_queue.lock", __FILE__, __LINE__);
    int LD = lockdep_register_class("dev_queue_xmit_nit", __FILE__, __LINE__);

    printf("Registered %d lock classes:\n", nclasses);
    for (int i = 0; i < nclasses; i++)
        printf("  [%d] %s\n", i, classes[i].name);
    printf("\n");

    /* ── Safe sequence 1: rtnl → socket_lock ─────────────────────────── */
    printf("\033[0;34m--- CPU 0: rtnl_lock -> socket_lock (net device ops) ---\033[0m\n");
    LOCK(LA);
    printf("  CPU0 acquired rtnl_lock\n");
    LOCK(LB);
    printf("  CPU0 acquired socket_lock (while holding rtnl_lock)\n");
    printf("  lockdep records edge: rtnl_lock -> socket_lock\n");
    UNLOCK(LB);
    UNLOCK(LA);
    printf("  Released both\n\n");

    /* ── Safe sequence 2: socket → sk_receive_queue ───────────────────── */
    printf("\033[0;34m--- CPU 0: socket_lock -> sk_receive_queue (recv path) ---\033[0m\n");
    LOCK(LB);
    printf("  CPU0 acquired socket_lock\n");
    LOCK(LC);
    printf("  CPU0 acquired sk_receive_queue.lock\n");
    printf("  lockdep records edge: socket_lock -> sk_receive_queue.lock\n");
    UNLOCK(LC);
    UNLOCK(LB);
    printf("  Released both\n\n");

    /* ── DANGEROUS: introduces cycle ──────────────────────────────────── */
    printf("\033[0;33m--- CPU 1: sk_receive_queue -> rtnl_lock (buggy path) ---\033[0m\n");
    printf("  This order reverses the chain: rtnl -> socket -> sk_recv\n");
    printf("  If CPU0 holds rtnl and CPU1 holds sk_recv, both will deadlock\n\n");

    LOCK(LC);
    printf("  CPU1 acquired sk_receive_queue.lock\n");
    printf("  CPU1 attempting rtnl_lock...\n");
    LOCK(LA); /* lockdep should fire here */
    UNLOCK(LA);
    UNLOCK(LC);

    /* ── Safe chaining: A -> D ──────────────────────────────────────────── */
    printf("\n\033[0;34m--- Normal: rtnl_lock -> dev_queue_xmit_nit ---\033[0m\n");
    LOCK(LA);
    LOCK(LD);
    printf("  CPU0 acquired both in safe order — no cycle\n");
    UNLOCK(LD);
    UNLOCK(LA);

    printf("\n\033[1;32m=== lockdep Summary ===\033[0m\n");
    printf("Lock classes   : %d\n", nclasses);
    printf("Edges recorded : %d\n", nedges);
    printf("Locks checked  : %d\n", locks_checked);
    printf("Warnings issued: %d\n", warnings);
    printf("\nDependency graph:\n");
    for (int i = 0; i < nedges; i++)
        printf("  %s -> %s\n", classes[edges[i].from_class].name,
               classes[edges[i].to_class].name);

    return (warnings > 0) ? 0 : 1; /* We expect warnings from the buggy path */
}
