/* SPDX-License-Identifier: GPL-2.0 */
/* sched_ext BPF helper definitions */

#ifndef __SCX_BPF_H__
#define __SCX_BPF_H__

/* Basic types */
#ifndef bool
#define bool _Bool
#endif

/* BPF Map Types */
#ifndef BPF_MAP_TYPE_ARRAY
#define BPF_MAP_TYPE_ARRAY 2
#endif
#ifndef BPF_MAP_TYPE_HASH
#define BPF_MAP_TYPE_HASH 1
#endif

/* BPF Map Flags */
#ifndef BPF_ANY
#define BPF_ANY 0
#endif

/* BPF Struct Ops Macro - expands to function definition */
#ifndef BPF_STRUCT_OPS
#define BPF_STRUCT_OPS(name, ...) name(__VA_ARGS__)
#endif

#ifndef BPF_STRUCT_OPS_SLEEPABLE
#define BPF_STRUCT_OPS_SLEEPABLE(name) name()
#endif

/* sched_ext BPF Helper Functions */
/* These are kernel-provided helpers, declared here for compilation */
/* Note: Actual implementations are provided by the kernel at runtime */
extern long scx_bpf_dispatch(void *p, unsigned long long dsq_id, 
			     unsigned long long slice, unsigned long long enq_flags);

extern bool scx_bpf_consume(unsigned long long dsq_id);

extern long scx_bpf_create_dsq(unsigned long long dsq_id, int node);

extern bool scx_bpf_test_and_clear_cpu_idle(int cpu);

extern int scx_bpf_pick_idle_cpu(void *cpus_ptr, unsigned long long flags);

/* Error codes */
#ifndef EINVAL
#define EINVAL 22
#endif

/* sched_ext_ops structure (simplified) */
struct sched_ext_ops {
	void *select_cpu;
	void *enqueue;
	void *dispatch;
	void *runnable;
	void *running;
	void *stopping;
	void *quiescent;
	void *enable;
	void *init;
	void *exit;
	char *name;
};

#endif /* __SCX_BPF_H__ */

