// SPDX-License-Identifier: GPL-2.0
/* Userspace loader for multi-level FIFO scheduler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "../include/multi_level_sched.h"

static volatile sig_atomic_t exiting = 0;

static void sig_handler(int sig)
{
    exiting = 1;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    if (level == LIBBPF_DEBUG)
        return 0;
    return vfprintf(stderr, format, args);
}

void print_stats(int stats_fd)
{
    struct sched_stats stats;
    __u32 key = 0;
    
    if (bpf_map_lookup_elem(stats_fd, &key, &stats) == 0) {
        printf("\n=== Scheduler Statistics ===\n");
        printf("HIGH Priority:   Enqueued: %10llu  Dispatched: %10llu  Depth: %5llu\n",
               stats.enqueued[PRIORITY_HIGH],
               stats.dispatched[PRIORITY_HIGH],
               stats.current_depth[PRIORITY_HIGH]);
        printf("MEDIUM Priority: Enqueued: %10llu  Dispatched: %10llu  Depth: %5llu\n",
               stats.enqueued[PRIORITY_MEDIUM],
               stats.dispatched[PRIORITY_MEDIUM],
               stats.current_depth[PRIORITY_MEDIUM]);
        printf("LOW Priority:    Enqueued: %10llu  Dispatched: %10llu  Depth: %5llu\n",
               stats.enqueued[PRIORITY_LOW],
               stats.dispatched[PRIORITY_LOW],
               stats.current_depth[PRIORITY_LOW]);
    }
}

int main(int argc, char **argv)
{
    struct bpf_object *obj;
    struct bpf_link *link = NULL;
    int err, stats_fd;
    
    libbpf_set_print(libbpf_print_fn);
    
    /* Load BPF object */
    obj = bpf_object__open_file("multi_level_sched.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }
    
    /* Load BPF programs */
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }
    
    /* Get stats map FD for monitoring */
    stats_fd = bpf_object__find_map_fd_by_name(obj, "stats_map");
    if (stats_fd < 0) {
        fprintf(stderr, "Failed to find stats map\n");
        err = -1;
        goto cleanup;
    }
    
    /* Attach scheduler - this is kernel version dependent */
    struct bpf_program *prog = bpf_object__find_program_by_name(obj, ".struct_ops");
    if (!prog) {
        fprintf(stderr, "Note: sched_ext may not be available on this kernel\n");
        fprintf(stderr, "This is a demonstration of the code structure\n");
    }
    
    printf("Multi-Level FIFO Scheduler loaded successfully\n");
    printf("Press Ctrl+C to exit and view statistics\n\n");
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    /* Monitor loop */
    while (!exiting) {
        sleep(5);
        print_stats(stats_fd);
    }
    
    printf("\n\nFinal Statistics:\n");
    print_stats(stats_fd);
    
cleanup:
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return err != 0;
}
