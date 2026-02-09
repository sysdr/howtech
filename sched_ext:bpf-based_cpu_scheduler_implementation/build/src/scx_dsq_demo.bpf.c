/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sched_ext DSQ Demo: Three-Priority BPF Scheduler
 * Demonstrates LOCAL, GLOBAL, and Custom DSQ usage
 */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "../include/scx_common.h"
#include "../include/scx_bpf.h"

char _license[] SEC("license") = "GPL";

/* Statistics map visible to userspace */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, u32);
    __type(value, struct dsq_stats);
    __uint(max_entries, 1);
} stats_map SEC(".maps");

/* Per-task priority map (normally would use BPF_MAP_TYPE_TASK_STORAGE) */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, u32);  // PID
    __type(value, u32); // Priority
    __uint(max_entries, 10240);
} task_priority SEC(".maps");

static inline struct dsq_stats *get_stats(void) {
    u32 key = 0;
    return bpf_map_lookup_elem(&stats_map, &key);
}

/*
 * Classify task priority based on nice value and other heuristics
 * In production: Use cgroups, task_struct fields, or custom policies
 */
static u32 classify_priority(struct task_struct *p) {
    /* Simple heuristic: use nice value */
    if (p->static_prio < 120)  // nice < 0 = high priority
        return PRIO_HIGH;
    else if (p->static_prio < 130) // nice 0-10 = normal
        return PRIO_NORMAL;
    return PRIO_LOW;
}

/*
 * select_cpu - Choose which CPU should run this task
 * Called before enqueue, can dispatch directly to LOCAL DSQ for fast path
 */
s32 BPF_STRUCT_OPS(scx_dsq_demo_select_cpu, struct task_struct *p, 
                   s32 prev_cpu, u64 wake_flags)
{
    struct dsq_stats *stats = get_stats();
    s32 cpu;
    
    /* Try to keep task on same CPU for cache locality */
    if (scx_bpf_test_and_clear_cpu_idle(prev_cpu)) {
        if (stats)
            __sync_fetch_and_add(&stats->cache_hits, 1);
        return prev_cpu;
    }
    
    /* Find any idle CPU */
    cpu = scx_bpf_pick_idle_cpu(p->cpus_ptr, 0);
    if (cpu >= 0)
        return cpu;
    
    /* No idle CPU, let enqueue/dispatch handle it */
    return prev_cpu;
}

/*
 * enqueue - Task became runnable, decide which DSQ to use
 * This is where our priority classification happens
 */
void BPF_STRUCT_OPS(scx_dsq_demo_enqueue, struct task_struct *p, u64 enq_flags)
{
    struct dsq_stats *stats = get_stats();
    u32 pid = p->pid;
    u32 prio;
    u64 dsq_id;
    
    /* Classify task priority */
    prio = classify_priority(p);
    bpf_map_update_elem(&task_priority, &pid, &prio, BPF_ANY);
    
    /* Choose DSQ based on priority */
    if (prio == PRIO_HIGH) {
        dsq_id = DSQ_HIGH;
    } else if (prio == PRIO_NORMAL) {
        dsq_id = DSQ_NORMAL;
    } else {
        dsq_id = DSQ_LOW;
    }
    
    /* Dispatch to appropriate custom DSQ */
    if (scx_bpf_dispatch(p, dsq_id, DEFAULT_SLICE_NS, enq_flags) == 0) {
        if (stats) {
            __sync_fetch_and_add(&stats->dispatched[prio], 1);
            __sync_fetch_and_add(&stats->custom_dispatches, 1);
        }
    } else {
        /* Dispatch failed, try GLOBAL as fallback */
        if (scx_bpf_dispatch(p, SCX_DSQ_GLOBAL, DEFAULT_SLICE_NS, enq_flags) == 0) {
            if (stats)
                __sync_fetch_and_add(&stats->global_dispatches, 1);
        } else {
            if (stats)
                __sync_fetch_and_add(&stats->dispatch_errors, 1);
        }
    }
}

/*
 * dispatch - Kernel needs work, tell it which DSQ to consume from
 * Priority order: HIGH → NORMAL → LOW → GLOBAL
 */
void BPF_STRUCT_OPS(scx_dsq_demo_dispatch, s32 cpu, struct task_struct *prev)
{
    struct dsq_stats *stats = get_stats();
    
    /* Consume from priority queues in order */
    if (scx_bpf_consume(DSQ_HIGH)) {
        if (stats)
            __sync_fetch_and_add(&stats->consumed[PRIO_HIGH], 1);
        return;
    }
    
    if (scx_bpf_consume(DSQ_NORMAL)) {
        if (stats)
            __sync_fetch_and_add(&stats->consumed[PRIO_NORMAL], 1);
        return;
    }
    
    if (scx_bpf_consume(DSQ_LOW)) {
        if (stats)
            __sync_fetch_and_add(&stats->consumed[PRIO_LOW], 1);
        return;
    }
    
    /* Finally consume from GLOBAL queue */
    scx_bpf_consume(SCX_DSQ_GLOBAL);
}

/*
 * runnable - Task transitioned to runnable state
 */
void BPF_STRUCT_OPS(scx_dsq_demo_runnable, struct task_struct *p, u64 enq_flags)
{
    /* Could track runnable state transitions here */
}

/*
 * running - Task started running on CPU
 */
void BPF_STRUCT_OPS(scx_dsq_demo_running, struct task_struct *p)
{
    /* Could track execution start here */
}

/*
 * stopping - Task about to stop running
 */
void BPF_STRUCT_OPS(scx_dsq_demo_stopping, struct task_struct *p, bool runnable)
{
    /* Could track execution end here */
}

/*
 * quiescent - CPU has no more work
 */
void BPF_STRUCT_OPS(scx_dsq_demo_quiescent, s32 cpu, u64 deq_flags)
{
    /* CPU idle, nothing to do */
}

/*
 * enable - Task is being enabled for this scheduler
 */
void BPF_STRUCT_OPS(scx_dsq_demo_enable, struct task_struct *p)
{
    /* Initialize task for our scheduler */
}

/*
 * init - Scheduler initialization
 */
s32 BPF_STRUCT_OPS_SLEEPABLE(scx_dsq_demo_init)
{
    /* Create our custom DSQs */
    if (scx_bpf_create_dsq(DSQ_HIGH, -1) < 0)
        return -EINVAL;
    
    if (scx_bpf_create_dsq(DSQ_NORMAL, -1) < 0)
        return -EINVAL;
        
    if (scx_bpf_create_dsq(DSQ_LOW, -1) < 0)
        return -EINVAL;
    
    bpf_printk("scx_dsq_demo initialized: 3 priority DSQs created\n");
    return 0;
}

/*
 * exit - Scheduler cleanup
 */
void BPF_STRUCT_OPS(scx_dsq_demo_exit, struct scx_exit_info *ei)
{
    bpf_printk("scx_dsq_demo exit: %s (reason: %d)\n", 
               ei->msg, ei->kind);
}

/*
 * Define our scheduler struct_ops
 */
SEC(".struct_ops")
struct sched_ext_ops scx_dsq_demo = {
    .select_cpu          = (void *)scx_dsq_demo_select_cpu,
    .enqueue             = (void *)scx_dsq_demo_enqueue,
    .dispatch            = (void *)scx_dsq_demo_dispatch,
    .runnable            = (void *)scx_dsq_demo_runnable,
    .running             = (void *)scx_dsq_demo_running,
    .stopping            = (void *)scx_dsq_demo_stopping,
    .quiescent           = (void *)scx_dsq_demo_quiescent,
    .enable              = (void *)scx_dsq_demo_enable,
    .init                = (void *)scx_dsq_demo_init,
    .exit                = (void *)scx_dsq_demo_exit,
    .name                = "scx_dsq_demo",
};
