// XDP program that FAILS verification
// ERROR: "invalid read from stack" or similar

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_fail_no_check(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    struct ethhdr *eth = data;
    
    // Missing bounds check here!
    // Should check: if ((void *)(eth + 1) > data_end) return XDP_DROP;
    
    // Direct access without verification - REJECTED
    if (eth->h_proto == __builtin_bswap16(0x0800)) {
        return XDP_PASS;
    }
    
    return XDP_DROP;
}

char _license[] SEC("license") = "GPL";
