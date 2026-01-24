// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "netcorr.skel.h"

#define TASK_COMM_LEN 16

struct event {
    __u32 pid;
    __u32 tgid;
    char comm[TASK_COMM_LEN];
    __u32 saddr;
    __u32 daddr;
    __u16 sport;
    __u16 dport;
    __u32 bytes;
    __u8 event_type;
};

static volatile bool exiting = false;

static void sig_handler(int sig)
{
    exiting = true;
}

static const char *event_type_str(__u8 type)
{
    switch (type) {
        case 1: return "CONNECT";
        case 2: return "SEND";
        case 3: return "RECV";
        case 4: return "CLOSE";
        default: return "UNKNOWN";
    }
}

static const char *event_color(__u8 type)
{
    switch (type) {
        case 1: return "\033[0;35m";  // Magenta for connect
        case 2: return "\033[0;32m";  // Green for send
        case 3: return "\033[0;36m";  // Cyan for recv
        case 4: return "\033[0;31m";  // Red for close
        default: return "\033[0m";
    }
}

static void ip_to_str(__u32 ip, char *buf)
{
    struct in_addr addr = { .s_addr = ip };
    inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN);
}

static int handle_event(void *ctx, void *data, size_t data_sz)
{
    const struct event *e = data;
    char saddr_str[INET_ADDRSTRLEN];
    char daddr_str[INET_ADDRSTRLEN];
    
    ip_to_str(e->saddr, saddr_str);
    ip_to_str(e->daddr, daddr_str);
    
    printf("%s%-8s\033[0m ", event_color(e->event_type), event_type_str(e->event_type));
    printf("PID: \033[1;33m%-6u\033[0m ", e->pid);
    printf("Comm: \033[1;34m%-16s\033[0m ", e->comm);
    printf("%s:%-5u -> %s:%-5u", 
           saddr_str, e->sport, daddr_str, e->dport);
    
    if (e->event_type == 2 || e->event_type == 3) {
        printf(" (\033[1;37m%u bytes\033[0m)", e->bytes);
    }
    
    printf("\n");
    fflush(stdout);
    
    return 0;
}

int main(int argc, char **argv)
{
    struct netcorr_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;

    libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
    libbpf_set_print(NULL);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("\033[1;36m");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("  eBPF Network Event Correlation Monitor\n");
    printf("  Tracking: tcp_connect, tcp_sendmsg, tcp_recvmsg, tcp_close\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\033[0m\n");

    skel = netcorr_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    err = netcorr_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load and verify BPF skeleton: %d\n", err);
        goto cleanup;
    }

    err = netcorr_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
        goto cleanup;
    }

    rb = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event, NULL, NULL);
    if (!rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    printf("Successfully started! Monitoring network events... (Ctrl-C to stop)\n\n");
    printf("\033[1;37m%-8s %-13s %-23s Connection\033[0m\n", 
           "Event", "PID", "Process");
    printf("────────────────────────────────────────────────────────────────────────────\n");

    while (!exiting) {
        err = ring_buffer__poll(rb, 100);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            fprintf(stderr, "Error polling ring buffer: %d\n", err);
            break;
        }
    }

    printf("\n\033[1;33mShutting down...\033[0m\n");

cleanup:
    ring_buffer__free(rb);
    netcorr_bpf__destroy(skel);
    return err < 0 ? -err : 0;
}
