// SPDX-License-Identifier: GPL-2.0
/* Multi-Level FIFO Scheduler using BPF_MAP_TYPE_QUEUE
 * 
 * Implements three priority levels:
 * - HIGH: Latency-sensitive tasks (nice < 0 or in high-priority cgroup)
 * - MEDIUM: Normal tasks (nice 0-10)
 * - LOW: Batch tasks (nice > 10)
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "multi_level_sched.h"

char _license[] SEC("license") = "GPL";

/* Priority level queues - BPF_MAP_TYPE_QUEUE provides FIFO semantics */
struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, MAX_QUEUE_SIZE);
    __type(value, u64); /* Store task PIDs */
} high_priority_queue SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, MAX_QUEUE_SIZE);
    __type(value, u64);
} medium_priority_queue SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, MAX_QUEUE_SIZE);
    __type(value, u64);
} low_priority_queue SEC(".maps");

/* Statistics for monitoring */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct sched_stats);
} stats_map SEC(".maps");

/* Per-task metadata for time slice tracking */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u64);   /* PID */
    __type(value, struct task_metadata);
} task_meta SEC(".maps");

/* Helper to update statistics atomically */
static void update_stat(enum priority_level level, bool enqueue)
{
    u32 key = 0;
    struct sched_stats *stats;
    
    stats = bpf_map_lookup_elem(&stats_map, &key);
    if (!stats)
        return;
    
    if (enqueue) {
        __sync_fetch_and_add(&stats->enqueued[level], 1);
        __sync_fetch_and_add(&stats->current_depth[level], 1);
    } else {
        __sync_fetch_and_add(&stats->dispatched[level], 1);
        if (stats->current_depth[level] > 0)
            __sync_fetch_and_sub(&stats->current_depth[level], 1);
    }
}

/* Determine priority level based on nice value */
static enum priority_level get_priority_level(struct task_struct *p)
{
    int prio = p->prio; /* Lower prio value = higher priority */
    
    /* Convert kernel priority to nice value
     * Kernel prio: 100-139 maps to nice -20 to +19
     * prio < 120 -> HIGH (nice < 0)
     * prio 120-130 -> MEDIUM (nice 0-10)  
     * prio > 130 -> LOW (nice > 10)
     */
    if (prio < 120)
        return PRIORITY_HIGH;
    else if (prio <= 130)
        return PRIORITY_MEDIUM;
    else
        return PRIORITY_LOW;
}

/* BPF program: select_cpu - Choose CPU for task */
SEC("struct_ops/select_cpu")
s32 BPF_PROG(mlq_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
    /* Use previous CPU to maintain cache locality */
    return prev_cpu;
}

/* BPF program: enqueue - Add task to appropriate priority queue */
SEC("struct_ops/enqueue")
void BPF_PROG(mlq_enqueue, struct task_struct *p, u64 enq_flags)
{
    enum priority_level level;
    u64 pid = p->pid;
    int ret;
    
    level = get_priority_level(p);
    
    /* Push PID to appropriate queue based on priority */
    switch (level) {
    case PRIORITY_HIGH:
        ret = bpf_map_push_elem(&high_priority_queue, &pid, 0);
        break;
    case PRIORITY_MEDIUM:
        ret = bpf_map_push_elem(&medium_priority_queue, &pid, 0);
        break;
    case PRIORITY_LOW:
        ret = bpf_map_push_elem(&low_priority_queue, &pid, 0);
        break;
    default:
        ret = -1;
    }
    
    if (ret == 0) {
        update_stat(level, true);
        
        /* Initialize task metadata for time slice tracking */
        struct task_metadata meta = {
            .priority = level,
            .time_slice_ns = DEFAULT_TIME_SLICE_NS,
            .dispatch_count = 0,
        };
        bpf_map_update_elem(&task_meta, &pid, &meta, BPF_ANY);
    }
    
    /* Always dispatch from enqueue for this simple implementation */
    scx_bpf_dispatch(p, SCX_DSQ_GLOBAL, SCX_SLICE_DFL, enq_flags);
}

/* BPF program: dispatch - Pull tasks from queues in priority order */
SEC("struct_ops/dispatch")
void BPF_PROG(mlq_dispatch, s32 cpu, struct task_struct *prev)
{
    u64 pid;
    struct task_struct *p;
    int i;
    
    /* Check high priority queue first - verifier requires bounded loop */
    for (i = 0; i < MAX_DISPATCH_BATCH; i++) {
        if (bpf_map_pop_elem(&high_priority_queue, &pid) == 0) {
            p = bpf_task_from_pid(pid);
            if (p) {
                scx_bpf_dispatch(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, 0);
                update_stat(PRIORITY_HIGH, false);
                bpf_task_release(p);
                continue;
            }
        }
        break;
    }
    
    /* Check medium priority queue */
    for (i = 0; i < MAX_DISPATCH_BATCH; i++) {
        if (bpf_map_pop_elem(&medium_priority_queue, &pid) == 0) {
            p = bpf_task_from_pid(pid);
            if (p) {
                scx_bpf_dispatch(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, 0);
                update_stat(PRIORITY_MEDIUM, false);
                bpf_task_release(p);
                continue;
            }
        }
        break;
    }
    
    /* Check low priority queue last */
    for (i = 0; i < MAX_DISPATCH_BATCH; i++) {
        if (bpf_map_pop_elem(&low_priority_queue, &pid) == 0) {
            p = bpf_task_from_pid(pid);
            if (p) {
                scx_bpf_dispatch(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, 0);
                update_stat(PRIORITY_LOW, false);
                bpf_task_release(p);
                continue;
            }
        }
        break;
    }
}

/* BPF program: running - Task is starting execution */
SEC("struct_ops/running")
void BPF_PROG(mlq_running, struct task_struct *p)
{
    u64 pid = p->pid;
    struct task_metadata *meta;
    
    meta = bpf_map_lookup_elem(&task_meta, &pid);
    if (meta) {
        meta->last_run_ns = bpf_ktime_get_ns();
        __sync_fetch_and_add(&meta->dispatch_count, 1);
    }
}

/* BPF program: stopping - Task is stopping execution */
SEC("struct_ops/stopping")
void BPF_PROG(mlq_stopping, struct task_struct *p, bool runnable)
{
    u64 pid = p->pid;
    struct task_metadata *meta;
    u64 now = bpf_ktime_get_ns();
    
    meta = bpf_map_lookup_elem(&task_meta, &pid);
    if (meta && meta->last_run_ns > 0) {
        u64 runtime = now - meta->last_run_ns;
        meta->total_runtime_ns += runtime;
    }
}

/* BPF program: init - Initialize scheduler */
SEC("struct_ops/init")
s32 BPF_PROG(mlq_init)
{
    /* Initialize statistics */
    u32 key = 0;
    struct sched_stats stats = {0};
    bpf_map_update_elem(&stats_map, &key, &stats, BPF_ANY);
    
    return 0;
}

/* BPF program: exit - Cleanup */
SEC("struct_ops/exit")
void BPF_PROG(mlq_exit, struct scx_exit_info *ei)
{
    /* Cleanup happens automatically via map deletion */
}

/* Scheduler operations structure */
SEC(".struct_ops")
struct sched_ext_ops mlq_ops = {
    .select_cpu     = (void *)mlq_select_cpu,
    .enqueue        = (void *)mlq_enqueue,
    .dispatch       = (void *)mlq_dispatch,
    .running        = (void *)mlq_running,
    .stopping       = (void *)mlq_stopping,
    .init           = (void *)mlq_init,
    .exit           = (void *)mlq_exit,
    .name           = "mlq_sched",
};
