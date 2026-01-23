// XDP program that FAILS verification
// ERROR: "math between pkt pointer and register with unbounded min value"

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_fail_unbounded(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    // Calculate offset WITHOUT bounds check
    unsigned long offset = 14; // Ethernet header
    void *ptr = data + offset;
    
    // ERROR: Verifier doesn't know if ptr < data_end
    // This will be REJECTED
    __u8 value = *(__u8 *)ptr;
    
    return value > 0 ? XDP_PASS : XDP_DROP;
}

char _license[] SEC("license") = "GPL";
