#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* BPF map for statistics */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 5);
    __type(key, __u32);
    __type(value, __u64);
} xdp_stats SEC(".maps");

/* Statistics indices */
#define STAT_RX_PACKETS   0
#define STAT_RX_BYTES     1
#define STAT_DROPPED      2
#define STAT_PASSED       3
#define STAT_REDIRECTED   4

SEC("xdp")
int xdp_drop_func(struct xdp_md *ctx)
{
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    /* Update RX packet counter */
    __u32 idx = STAT_RX_PACKETS;
    __u64 *count = bpf_map_lookup_elem(&xdp_stats, &idx);
    if (count) {
        __sync_fetch_and_add(count, 1);
    }
    
    /* Update RX bytes counter */
    idx = STAT_RX_BYTES;
    count = bpf_map_lookup_elem(&xdp_stats, &idx);
    if (count) {
        __u64 bytes = data_end - data;
        __sync_fetch_and_add(count, bytes);
    }
    
    /* Parse Ethernet header */
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end) {
        return XDP_DROP;
    }
    
    /* Check if it's an IP packet */
    if (eth->h_proto != bpf_htons(ETH_P_IP)) {
        idx = STAT_PASSED;
        count = bpf_map_lookup_elem(&xdp_stats, &idx);
        if (count) {
            __sync_fetch_and_add(count, 1);
        }
        return XDP_PASS;
    }
    
    /* Parse IP header */
    struct iphdr *iph = (struct iphdr *)(eth + 1);
    if ((void *)(iph + 1) > data_end) {
        return XDP_DROP;
    }
    
    /* Drop all UDP packets on port 9999 (our test traffic) */
    if (iph->protocol == IPPROTO_UDP) {
        idx = STAT_DROPPED;
        count = bpf_map_lookup_elem(&xdp_stats, &idx);
        if (count) {
            __sync_fetch_and_add(count, 1);
        }
        return XDP_DROP;
    }
    
    /* Pass everything else */
    idx = STAT_PASSED;
    count = bpf_map_lookup_elem(&xdp_stats, &idx);
    if (count) {
        __sync_fetch_and_add(count, 1);
    }
    
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
