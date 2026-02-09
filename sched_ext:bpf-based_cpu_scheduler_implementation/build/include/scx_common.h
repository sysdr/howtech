#ifndef __SCX_COMMON_H
#define __SCX_COMMON_H

/* Define types ourselves for BPF compatibility */
/* In BPF context, vmlinux.h should be included first and define these */
/* In userspace, we define them here */
#ifndef __VMLINUX_H__
/* Userspace definitions - define manually to avoid system headers in BPF */
typedef unsigned char __u8;
typedef signed char __s8;
typedef unsigned short __u16;
typedef signed short __s16;
typedef unsigned int __u32;
typedef signed int __s32;
typedef unsigned long long __u64;
typedef signed long long __s64;
#endif

/* DSQ IDs for our three-priority scheduler */
#define DSQ_HIGH     0
#define DSQ_NORMAL   1  
#define DSQ_LOW      2
#define DSQ_GLOBAL   0xffffffffffffffffULL

/* Task priority classification */
#define PRIO_HIGH    0
#define PRIO_NORMAL  1
#define PRIO_LOW     2

/* Default timeslice in nanoseconds */
#define DEFAULT_SLICE_NS (5000000ULL)  // 5ms

/* Stats structure shared between BPF and userspace */
struct dsq_stats {
    __u64 dispatched[3];      // Per-priority dispatch counts
    __u64 consumed[3];         // Per-priority consume counts
    __u64 local_dispatches;    // LOCAL DSQ usage
    __u64 global_dispatches;   // GLOBAL DSQ usage
    __u64 custom_dispatches;   // Custom DSQ usage
    __u64 dispatch_errors;     // Failed dispatches
    __u64 ipis_sent;          // Cross-CPU IPIs
    __u64 cache_hits;         // Same-CPU dispatch (good locality)
};

#endif /* __SCX_COMMON_H */
