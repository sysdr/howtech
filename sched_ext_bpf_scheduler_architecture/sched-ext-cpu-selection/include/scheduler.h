#ifndef __SCHEDULER_H
#define __SCHEDULER_H

struct latency_event {
    __u32 pid;
    __u32 group_id;
    __u64 latency_ns;
    __u32 cpu;
};

struct group_stats {
    __u64 wakeup_count;
    __u64 total_latency_ns;
    __u64 cache_hits;
    __u64 cache_misses;
};

#endif /* __SCHEDULER_H */
