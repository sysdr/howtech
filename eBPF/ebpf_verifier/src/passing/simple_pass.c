// Simple XDP program that PASSES verification
// Demonstrates proper bounds checking

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_pass_simple(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    // Calculate Ethernet header end
    struct ethhdr *eth = data;
    
    // CRITICAL: Bounds check BEFORE access
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    
    // Now safe to access eth header
    // Verifier has proven: data <= eth < eth+1 <= data_end
    if (eth->h_proto == __builtin_bswap16(0x0800)) { // IPv4
        return XDP_PASS;
    }
    
    return XDP_DROP;
}

char _license[] SEC("license") = "GPL";
