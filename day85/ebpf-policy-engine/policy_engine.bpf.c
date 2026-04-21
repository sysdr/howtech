// SPDX-License-Identifier: GPL-2.0
// policy_engine.bpf.c — TC ingress policy enforcement
#if defined(__clang__) && defined(__BPF__)
/* Older distro headers use gcc-only spellings clang BPF does not know */
#ifndef __no_sanitize_or_inline
#define __no_sanitize_or_inline
#endif
#ifndef __no_kasan_or_inline
#define __no_kasan_or_inline
#endif
#endif
#include <linux/types.h>
typedef __kernel_size_t size_t;
typedef _Bool bool;
#ifndef true
#define true ((bool)1)
#endif
#ifndef false
#define false ((bool)0)
#endif
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>
#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

#define VERDICT_ALLOW 1
#define VERDICT_DENY  0

struct policy_key {
    __u32 src_ip;    /* 0 = wildcard */
    __u16 dst_port;  /* 0 = any */
    __u8  protocol;
    __u8  pad;
};

struct policy_entry {
    __u8  action;
    __u8  _pad[3];
    __u32 hits;
};

struct pkt_event {
    __u32 src_ip;
    __u32 dst_ip;
    __u16 src_port;
    __u16 dst_port;
    __u8  protocol;
    __u8  action;
    __u16 _pad;
    __u64 ts_ns;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 256);
    __type(key,   struct policy_key);
    __type(value, struct policy_entry);
} policy_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 20);   /* 1 MB */
} events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 2);
    __type(key,   __u32);
    __type(value, __u64);
} stats SEC(".maps");

static __always_inline __u8
check_policy(__u32 src_ip, __u16 dst_port, __u8 proto)
{
    struct policy_key k = { .src_ip = src_ip, .dst_port = dst_port,
                             .protocol = proto, .pad = 0 };
    struct policy_entry *e = bpf_map_lookup_elem(&policy_map, &k);
    if (e) {
        __sync_fetch_and_add(&e->hits, 1);
        return e->action;
    }
    /* wildcard source */
    k.src_ip = 0;
    e = bpf_map_lookup_elem(&policy_map, &k);
    if (e) {
        __sync_fetch_and_add(&e->hits, 1);
        return e->action;
    }
    return VERDICT_ALLOW;   /* default allow */
}

SEC("classifier")
int policy_engine_fn(struct __sk_buff *skb)
{
    void *data     = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return TC_ACT_OK;
    if (bpf_ntohs(eth->h_proto) != 0x0800)   /* ETH_P_IP */
        return TC_ACT_OK;

    struct iphdr *ip = (struct iphdr *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return TC_ACT_OK;
    if (ip->ihl < 5)
        return TC_ACT_OK;

    __u16 src_port = 0, dst_port = 0;
    void *l4 = (void *)ip + ((__u32)ip->ihl * 4);

    if (ip->protocol == 6) {        /* TCP */
        struct tcphdr *tcp = l4;
        if ((void *)(tcp + 1) > data_end)
            return TC_ACT_OK;
        src_port = bpf_ntohs(tcp->source);
        dst_port = bpf_ntohs(tcp->dest);
    } else if (ip->protocol == 17) { /* UDP */
        struct udphdr *udp = l4;
        if ((void *)(udp + 1) > data_end)
            return TC_ACT_OK;
        src_port = bpf_ntohs(udp->source);
        dst_port = bpf_ntohs(udp->dest);
    }

    __u8 action = check_policy(ip->saddr, dst_port, ip->protocol);

    struct pkt_event *ev = bpf_ringbuf_reserve(&events, sizeof(*ev), 0);
    if (ev) {
        ev->src_ip   = ip->saddr;
        ev->dst_ip   = ip->daddr;
        ev->src_port = src_port;
        ev->dst_port = dst_port;
        ev->protocol = ip->protocol;
        ev->action   = action;
        ev->_pad     = 0;
        ev->ts_ns    = bpf_ktime_get_ns();
        bpf_ringbuf_submit(ev, 0);
    }

    __u32 idx = (action == VERDICT_ALLOW) ? 0 : 1;
    __u64 *cnt = bpf_map_lookup_elem(&stats, &idx);
    if (cnt)
        __sync_fetch_and_add(cnt, 1);

    return (action == VERDICT_ALLOW) ? TC_ACT_OK : TC_ACT_SHOT;
}

char LICENSE[] SEC("license") = "GPL";
