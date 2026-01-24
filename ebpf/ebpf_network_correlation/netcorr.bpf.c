// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <linux/ptrace.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define TASK_COMM_LEN 16
#define AF_INET 2
#define AF_INET6 10

struct conn_info {
    __u32 pid;
    __u32 tgid;
    char comm[TASK_COMM_LEN];
    __u64 timestamp;
    __u32 saddr;
    __u32 daddr;
    __u16 sport;
    __u16 dport;
};

struct event {
    __u32 pid;
    __u32 tgid;
    char comm[TASK_COMM_LEN];
    __u32 saddr;
    __u32 daddr;
    __u16 sport;
    __u16 dport;
    __u32 bytes;
    __u8 event_type; // 1=connect, 2=send, 3=recv, 4=close
};

// Maps
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u64);   // socket pointer
    __type(value, struct conn_info);
} conn_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

// Helper to extract IP and port from socket
static __always_inline void extract_sock_info(struct sock *sk, 
                                               __u32 *saddr, __u32 *daddr,
                                               __u16 *sport, __u16 *dport) {
    __u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);
    
    if (family == AF_INET) {
        *saddr = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr);
        *daddr = BPF_CORE_READ(sk, __sk_common.skc_daddr);
        *sport = BPF_CORE_READ(sk, __sk_common.skc_num);
        *dport = bpf_ntohs(BPF_CORE_READ(sk, __sk_common.skc_dport));
    }
}

SEC("fentry/tcp_connect")
int BPF_PROG(trace_tcp_connect, struct sock *sk)
{
    __u64 sock_ptr = (__u64)sk;
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    
    struct conn_info info = {};
    info.pid = pid_tgid >> 32;
    info.tgid = pid_tgid;
    info.timestamp = bpf_ktime_get_ns();
    bpf_get_current_comm(&info.comm, sizeof(info.comm));
    
    extract_sock_info(sk, &info.saddr, &info.daddr, &info.sport, &info.dport);
    
    bpf_map_update_elem(&conn_map, &sock_ptr, &info, BPF_ANY);
    
    // Emit connect event
    struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (e) {
        e->pid = info.pid;
        e->tgid = info.tgid;
        __builtin_memcpy(e->comm, info.comm, TASK_COMM_LEN);
        e->saddr = info.saddr;
        e->daddr = info.daddr;
        e->sport = info.sport;
        e->dport = info.dport;
        e->bytes = 0;
        e->event_type = 1;
        bpf_ringbuf_submit(e, 0);
    }
    
    return 0;
}

SEC("fentry/tcp_sendmsg")
int BPF_PROG(trace_tcp_sendmsg, struct sock *sk, struct msghdr *msg, size_t size)
{
    __u64 sock_ptr = (__u64)sk;
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 current_pid = pid_tgid >> 32;
    
    struct conn_info *info = bpf_map_lookup_elem(&conn_map, &sock_ptr);
    if (!info) {
        // Socket not tracked, create minimal entry
        struct conn_info new_info = {};
        new_info.pid = current_pid;
        new_info.tgid = pid_tgid;
        new_info.timestamp = bpf_ktime_get_ns();
        bpf_get_current_comm(&new_info.comm, sizeof(new_info.comm));
        extract_sock_info(sk, &new_info.saddr, &new_info.daddr, 
                         &new_info.sport, &new_info.dport);
        bpf_map_update_elem(&conn_map, &sock_ptr, &new_info, BPF_ANY);
        info = &new_info;
    }
    
    // Check for PID change (fork/exec case)
    if (info->pid != current_pid) {
        info->pid = current_pid;
        bpf_get_current_comm(&info->comm, sizeof(info->comm));
        bpf_map_update_elem(&conn_map, &sock_ptr, info, BPF_ANY);
    }
    
    // Emit send event
    struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (e) {
        e->pid = info->pid;
        e->tgid = info->tgid;
        __builtin_memcpy(e->comm, info->comm, TASK_COMM_LEN);
        e->saddr = info->saddr;
        e->daddr = info->daddr;
        e->sport = info->sport;
        e->dport = info->dport;
        e->bytes = size;
        e->event_type = 2;
        bpf_ringbuf_submit(e, 0);
    }
    
    return 0;
}

SEC("fentry/tcp_recvmsg")
int BPF_PROG(trace_tcp_recvmsg, struct sock *sk, struct msghdr *msg, 
             size_t len, int flags, int *addr_len)
{
    __u64 sock_ptr = (__u64)sk;
    
    struct conn_info *info = bpf_map_lookup_elem(&conn_map, &sock_ptr);
    if (!info)
        return 0;
    
    // Emit recv event
    struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (e) {
        e->pid = info->pid;
        e->tgid = info->tgid;
        __builtin_memcpy(e->comm, info->comm, TASK_COMM_LEN);
        e->saddr = info->saddr;
        e->daddr = info->daddr;
        e->sport = info->sport;
        e->dport = info->dport;
        e->bytes = len;
        e->event_type = 3;
        bpf_ringbuf_submit(e, 0);
    }
    
    return 0;
}

SEC("fentry/tcp_close")
int BPF_PROG(trace_tcp_close, struct sock *sk, long timeout)
{
    __u64 sock_ptr = (__u64)sk;
    
    struct conn_info *info = bpf_map_lookup_elem(&conn_map, &sock_ptr);
    if (info) {
        // Emit close event
        struct event *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
        if (e) {
            e->pid = info->pid;
            e->tgid = info->tgid;
            __builtin_memcpy(e->comm, info->comm, TASK_COMM_LEN);
            e->saddr = info->saddr;
            e->daddr = info->daddr;
            e->sport = info->sport;
            e->dport = info->dport;
            e->bytes = 0;
            e->event_type = 4;
            bpf_ringbuf_submit(e, 0);
        }
        
        // Delete from map to prevent leak
        bpf_map_delete_elem(&conn_map, &sock_ptr);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
