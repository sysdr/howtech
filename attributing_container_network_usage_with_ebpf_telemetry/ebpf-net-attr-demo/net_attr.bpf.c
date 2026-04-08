// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include <bpf/bpf_helpers.h>

struct net_stats {
    __u64 rx_bytes;
    __u64 tx_bytes;
    __u64 rx_packets;
    __u64 tx_packets;
};

struct cgroup_event {
    __u64 cgroupid;
    __u64 ts_ns;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_HASH);
    __uint(max_entries, 512);
    __type(key, __u64);
    __type(value, struct net_stats);
} stats_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 17);
} events SEC(".maps");

SEC("tc")
int tc_egress(struct __sk_buff *skb)
{
    __u64 cgid = bpf_skb_cgroup_id(skb);
    if (cgid == 0)
        cgid = bpf_get_current_cgroup_id();

    struct net_stats *st = bpf_map_lookup_elem(&stats_map, &cgid);
    if (st) {
        st->tx_bytes   += (__u64)skb->len;
        st->tx_packets += 1;
    } else {
        struct net_stats zero = {};
        zero.tx_bytes   = (__u64)skb->len;
        zero.tx_packets = 1;
        bpf_map_update_elem(&stats_map, &cgid, &zero, BPF_NOEXIST);
        struct cgroup_event *ev =
            bpf_ringbuf_reserve(&events, sizeof(*ev), 0);
        if (ev) {
            ev->cgroupid = cgid;
            ev->ts_ns    = bpf_ktime_get_ns();
            bpf_ringbuf_submit(ev, 0);
        }
    }
    return TC_ACT_OK;
}

SEC("tc")
int tc_ingress(struct __sk_buff *skb)
{
    __u64 cgid = bpf_skb_cgroup_id(skb);
    if (cgid == 0)
        cgid = bpf_get_current_cgroup_id();

    struct net_stats *st = bpf_map_lookup_elem(&stats_map, &cgid);
    if (st) {
        st->rx_bytes   += (__u64)skb->len;
        st->rx_packets += 1;
    } else {
        struct net_stats zero = {};
        zero.rx_bytes   = (__u64)skb->len;
        zero.rx_packets = 1;
        bpf_map_update_elem(&stats_map, &cgid, &zero, BPF_NOEXIST);
    }
    return TC_ACT_OK;
}

char LICENSE[] SEC("license") = "GPL";
