// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <signal.h>

static volatile sig_atomic_t stop = 0;

static void sig_handler(int sig) {
    stop = 1;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, 
                          va_list args) {
    return vfprintf(stderr, format, args);
}

int main(int argc, char **argv) {
    struct bpf_object *obj;
    struct bpf_link *link_open = NULL, *link_exec = NULL;
    int err, blocked_inodes_fd, allowed_uids_fd, stats_fd;
    struct stat st;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path_to_block>\n", argv[0]);
        return 1;
    }

    /* Set up libbpf errors and debug info callback */
    libbpf_set_print(libbpf_print_fn);

    /* Bump RLIMIT_MEMLOCK to allow BPF sub-system to do anything */
    struct rlimit rlim_new = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };
    if (setrlimit(RLIMIT_MEMLOCK, &rlim_new)) {
        fprintf(stderr, "Failed to increase RLIMIT_MEMLOCK limit: %s\n",
                strerror(errno));
        return 1;
    }

    /* Load BPF object file */
    obj = bpf_object__open_file("build/file_policy.bpf.o", NULL);
    err = libbpf_get_error(obj);
    if (err) {
        fprintf(stderr, "Failed to open BPF object: %s\n", strerror(-err));
        return 1;
    }

    /* Load BPF object into kernel */
    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %s\n", strerror(-err));
        goto cleanup;
    }

    /* Get file descriptor for maps */
    blocked_inodes_fd = bpf_object__find_map_fd_by_name(obj, "blocked_inodes");
    allowed_uids_fd = bpf_object__find_map_fd_by_name(obj, "allowed_uids");
    stats_fd = bpf_object__find_map_fd_by_name(obj, "policy_stats");

    if (blocked_inodes_fd < 0 || allowed_uids_fd < 0 || stats_fd < 0) {
        fprintf(stderr, "Failed to get map file descriptors\n");
        err = -1;
        goto cleanup;
    }

    /* Get inode of file to block */
    if (stat(argv[1], &st) != 0) {
        fprintf(stderr, "Failed to stat %s: %s\n", argv[1], strerror(errno));
        err = 1;
        goto cleanup;
    }

    printf("Blocking file: %s (inode: %lu)\n", argv[1], st.st_ino);

    /* Add inode to blocked list */
    __u64 inode = st.st_ino;
    __u32 blocked = 1;
    err = bpf_map_update_elem(blocked_inodes_fd, &inode, &blocked, BPF_ANY);
    if (err) {
        fprintf(stderr, "Failed to update blocked_inodes map: %s\n", 
                strerror(-err));
        goto cleanup;
    }

    /* Allow root (UID 0) to bypass restrictions */
    __u32 root_uid = 0;
    __u32 allowed = 1;
    err = bpf_map_update_elem(allowed_uids_fd, &root_uid, &allowed, BPF_ANY);
    if (err) {
        fprintf(stderr, "Failed to update allowed_uids map: %s\n", 
                strerror(-err));
        goto cleanup;
    }

    /* Attach LSM programs */
    struct bpf_program *prog_open, *prog_exec;
    
    prog_open = bpf_object__find_program_by_name(obj, "restrict_file_open");
    if (!prog_open) {
        fprintf(stderr, "Failed to find file_open program\n");
        err = -1;
        goto cleanup;
    }

    prog_exec = bpf_object__find_program_by_name(obj, "restrict_exec");
    if (!prog_exec) {
        fprintf(stderr, "Failed to find bprm_check program\n");
        err = -1;
        goto cleanup;
    }

    link_open = bpf_program__attach(prog_open);
    err = libbpf_get_error(link_open);
    if (err) {
        fprintf(stderr, "Failed to attach file_open program: %s\n", 
                strerror(-err));
        fprintf(stderr, "Note: LSM BPF requires kernel 5.7+ with CONFIG_BPF_LSM=y\n");
        link_open = NULL;
        goto cleanup;
    }

    link_exec = bpf_program__attach(prog_exec);
    err = libbpf_get_error(link_exec);
    if (err) {
        fprintf(stderr, "Failed to attach bprm_check program: %s\n", 
                strerror(-err));
        link_exec = NULL;
        goto cleanup;
    }

    printf("\n✓ LSM BPF programs loaded and attached successfully!\n");
    printf("✓ Policy enforced: %s is now protected\n", argv[1]);
    printf("✓ Only UID 0 (root) can access this file\n\n");
    printf("Press Ctrl+C to stop and unload...\n\n");

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* Keep running and print stats periodically */
    while (!stop) {
        sleep(5);
        
        __u32 key = 0;
        struct {
            __u64 total_checks;
            __u64 denied_count;
            __u64 allowed_count;
        } stats;

        if (bpf_map_lookup_elem(stats_fd, &key, &stats) == 0) {
            printf("\rPolicy Stats - Total: %llu | Allowed: %llu | Denied: %llu   ",
                   stats.total_checks, stats.allowed_count, stats.denied_count);
            fflush(stdout);
        }
    }

    printf("\n\nCleaning up...\n");

cleanup:
    bpf_link__destroy(link_open);
    bpf_link__destroy(link_exec);
    bpf_object__close(obj);
    return err != 0;
}
