// Reference-only snippet for the article/demo text.
// This file is not built in this repo; it exists to show the revert trigger
// pattern used by real sched_ext BPF schedulers.

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

static __u64 enqueue_count;

SEC("struct_ops/scx_ops_enqueue")
void BPF_PROG(scx_ops_enqueue, struct task_struct *p, u64 enq_flags)
{
    if (++enqueue_count > 100) {
        /* deliberate revert: count=N */
        scx_bpf_error("deliberate revert: count=%llu", enqueue_count);
    }
}
