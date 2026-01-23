// XDP program with bounded loop that PASSES verification
// Demonstrates explicit loop bounds for verifier

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <bpf/bpf_helpers.h>

#define MAX_ITERATIONS 16

SEC("xdp")
int xdp_bounded_loop(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    struct ethhdr *eth = data;
    
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    
    // Bounded loop with explicit counter
    // Verifier can prove this terminates in MAX_ITERATIONS
    unsigned char *payload = (unsigned char *)(eth + 1);
    __u32 sum = 0;
    
    #pragma unroll
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        if ((void *)(payload + i + 1) > data_end)
            break;
        sum += payload[i];
    }
    
    // Use sum to prevent optimization
    return sum > 0 ? XDP_PASS : XDP_DROP;
}

char _license[] SEC("license") = "GPL";
