// SPDX-License-Identifier: GPL-2.0
/* CPU Selection Scheduler with Dynamic Grouping */

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "scheduler.h"

char _license[] SEC("license") = "GPL";

/* Group membership: PID -> group_id */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, u32);
} group_map SEC(".maps");

/* Per-CPU load counters */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 256);
    __type(key, u32);
    __type(value, u64);
} cpu_load SEC(".maps");

/* Last CPU each task ran on */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, u32);
} task_last_cpu SEC(".maps");

/* Wakeup latency events */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

/* Statistics export */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 2);
    __type(key, u32);
    __type(value, struct group_stats);
} stats_map SEC(".maps");

/* Wakeup timestamps */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, u64);
} wakeup_time SEC(".maps");

static __always_inline u32 get_task_group(u32 pid)
{
    u32 *group = bpf_map_lookup_elem(&group_map, &pid);
    return group ? *group : 1; /* Default to batch group */
}

static __always_inline u64 get_cpu_load(u32 cpu)
{
    u64 *load = bpf_map_lookup_elem(&cpu_load, &cpu);
    return load ? *load : 0;
}

static __always_inline void inc_cpu_load(u32 cpu)
{
    u64 *load = bpf_map_lookup_elem(&cpu_load, &cpu);
    if (load)
        __sync_fetch_and_add(load, 1);
}

static __always_inline u32 select_cache_warm_cpu(u32 pid, u32 group_id)
{
    u32 *last_cpu = bpf_map_lookup_elem(&task_last_cpu, &pid);
    if (!last_cpu)
        return 0xFFFFFFFF;
    
    u32 cpu = *last_cpu;
    u64 load = get_cpu_load(cpu);
    
    /* Interactive tasks get aggressive cache preference */
    u64 threshold = (group_id == 0) ? 10 : 5;
    
    if (load < threshold)
        return cpu;
    
    return 0xFFFFFFFF;
}

static __always_inline u32 select_least_loaded_cpu(u32 nr_cpus)
{
    u32 best_cpu = 0;
    u64 min_load = 0xFFFFFFFFFFFFFFFFULL;
    
    /* Bounded loop for BPF verifier */
    #pragma unroll
    for (u32 cpu = 0; cpu < 64 && cpu < nr_cpus; cpu++) {
        u64 load = get_cpu_load(cpu);
        if (load < min_load) {
            min_load = load;
            best_cpu = cpu;
        }
    }
    
    return best_cpu;
}

SEC("struct_ops/select_cpu")
s32 BPF_PROG(select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
    u32 pid = p->pid;
    u32 group_id = get_task_group(pid);
    u32 nr_cpus = 64; /* Adjust based on system */
    s32 cpu;
    
    /* Try cache-warm CPU first */
    cpu = select_cache_warm_cpu(pid, group_id);
    if (cpu != 0xFFFFFFFF && cpu < nr_cpus) {
        inc_cpu_load(cpu);
        return cpu;
    }
    
    /* Fall back to load balancing */
    cpu = select_least_loaded_cpu(nr_cpus);
    inc_cpu_load(cpu);
    
    return cpu;
}

SEC("struct_ops/enqueue")
void BPF_PROG(enqueue, struct task_struct *p, u64 enq_flags)
{
    u32 pid = p->pid;
    u64 now = bpf_ktime_get_ns();
    
    /* Record wakeup time for latency measurement */
    bpf_map_update_elem(&wakeup_time, &pid, &now, BPF_ANY);
}

SEC("struct_ops/dispatch")
void BPF_PROG(dispatch, s32 cpu, struct task_struct *prev)
{
    /* Default dispatch - kernel handles task selection */
}

SEC("struct_ops/running")
void BPF_PROG(running, struct task_struct *p)
{
    u32 pid = p->pid;
    u32 cpu = bpf_get_smp_processor_id();
    
    /* Update last CPU for cache tracking */
    bpf_map_update_elem(&task_last_cpu, &pid, &cpu, BPF_ANY);
    
    /* Measure wakeup latency */
    u64 *wakeup = bpf_map_lookup_elem(&wakeup_time, &pid);
    if (wakeup) {
        u64 now = bpf_ktime_get_ns();
        u64 latency_ns = now - *wakeup;
        
        struct latency_event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
        if (e) {
            e->pid = pid;
            e->group_id = get_task_group(pid);
            e->latency_ns = latency_ns;
            e->cpu = cpu;
            bpf_ringbuf_submit(e, 0);
        }
        
        bpf_map_delete_elem(&wakeup_time, &pid);
    }
}

SEC("struct_ops/stopping")
void BPF_PROG(stopping, struct task_struct *p, bool runnable)
{
    /* Task stopped, nothing special to do */
}

SEC("struct_ops/enable")
void BPF_PROG(enable, struct task_struct *p)
{
    /* Task enabled in this scheduler */
}

SEC("struct_ops/init")
s32 BPF_PROG(init)
{
    return 0;
}

SEC("struct_ops/exit")
void BPF_PROG(sched_exit, struct scx_exit_info *info)
{
    /* Cleanup on exit */
}

SEC(".struct_ops")
struct sched_ext_ops cpu_select_ops = {
    .select_cpu = (void *)select_cpu,
    .enqueue = (void *)enqueue,
    .dispatch = (void *)dispatch,
    .running = (void *)running,
    .stopping = (void *)stopping,
    .enable = (void *)enable,
    .init = (void *)init,
    .exit = (void *)sched_exit,
    .name = "cpu_select",
};
