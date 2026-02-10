#ifndef __MULTI_LEVEL_SCHED_H
#define __MULTI_LEVEL_SCHED_H

#define MAX_QUEUE_SIZE 1024
#define MAX_DISPATCH_BATCH 32
#define DEFAULT_TIME_SLICE_NS 10000000  /* 10ms */

enum priority_level {
    PRIORITY_HIGH = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_LOW = 2,
    PRIORITY_MAX = 3,
};

struct sched_stats {
    __u64 enqueued[PRIORITY_MAX];
    __u64 dispatched[PRIORITY_MAX];
    __u64 current_depth[PRIORITY_MAX];
};

struct task_metadata {
    __u32 priority;
    __u64 time_slice_ns;
    __u64 last_run_ns;
    __u64 total_runtime_ns;
    __u32 dispatch_count;
};

#endif /* __MULTI_LEVEL_SCHED_H */
