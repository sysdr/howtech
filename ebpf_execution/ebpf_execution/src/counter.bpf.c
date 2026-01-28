// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/*
 * Simple counter program for benchmarking JIT vs interpreter
 * This program demonstrates the performance difference between
 * interpreted bytecode and JIT-compiled native code.
 */

SEC("socket")
int counter_prog(struct __sk_buff *skb)
{
    volatile __u64 counter = 0;

    /*
     * Heavier arithmetic workload to better expose JIT vs interpreter
     * differences, especially on environments like WSL where syscall
     * overhead can dominate very small programs.
     *
     * Still fully bounded (required by the verifier).
     */
    #pragma unroll
    for (int i = 0; i < 512; i++) {
        __u64 v = i;

        /* Mix of adds, multiplies and bit ops */
        v = v * 17 + 23;
        v ^= (v << 5);
        v ^= (v >> 7);
        v += (v << 3);

        counter += v;
    }

    return counter;
}

SEC("xdp")
int xdp_counter(struct xdp_md *ctx)
{
    volatile __u64 sum = 0;
    __u64 start = ctx->data;
    __u64 end = ctx->data_end;
    
    // Compute simple checksum-like operation
    #pragma unroll
    for (int i = 0; i < 50; i++) {
        sum += i * 7;
        sum ^= (sum << 3);
        sum ^= (sum >> 5);
    }
    
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
