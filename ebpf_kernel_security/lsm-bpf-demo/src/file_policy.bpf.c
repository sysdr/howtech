// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/binfmts.h>
#include <stddef.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "GPL";

/* BPF map to store blocked inodes */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __u64);      /* inode number */
    __type(value, __u32);    /* 1 = blocked */
    __uint(max_entries, 1024);
} blocked_inodes SEC(".maps");

/* BPF map to store allowed UIDs */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __u32);      /* UID */
    __type(value, __u32);    /* 1 = allowed */
    __uint(max_entries, 256);
} allowed_uids SEC(".maps");

/* Statistics map */
struct stats {
    __u64 total_checks;
    __u64 denied_count;
    __u64 allowed_count;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __type(key, __u32);
    __type(value, struct stats);
    __uint(max_entries, 1);
} policy_stats SEC(".maps");

/* LSM hook for file open operations */
SEC("lsm/file_open")
int BPF_PROG(restrict_file_open, struct file *file, int ret)
{
    if (ret != 0)
        return ret;  /* Previous LSM denied, respect that */

    __u32 stats_key = 0;
    struct stats *s = bpf_map_lookup_elem(&policy_stats, &stats_key);
    if (s)
        __sync_fetch_and_add(&s->total_checks, 1);

    /* Get current UID */
    __u64 uid_gid = bpf_get_current_uid_gid();
    __u32 uid = uid_gid & 0xFFFFFFFF;

    /* Check if UID is in allowed list */
    __u32 *allowed = bpf_map_lookup_elem(&allowed_uids, &uid);
    
    /* Get file inode - using pointer arithmetic (f_inode is typically at offset 0x20) */
    struct inode *inode = NULL;
    __u64 ino = 0;
    void *f_inode_ptr = (char *)file + 0x20;  /* Approximate offset for f_inode */
    bpf_probe_read_kernel(&inode, sizeof(inode), f_inode_ptr);
    if (inode) {
        void *i_ino_ptr = (char *)inode + 0x38;  /* Approximate offset for i_ino */
        bpf_probe_read_kernel(&ino, sizeof(ino), i_ino_ptr);
    }

    /* Check if this inode is blocked */
    __u32 *blocked = bpf_map_lookup_elem(&blocked_inodes, &ino);
    
    if (blocked && *blocked == 1) {
        /* File is in block list */
        if (!allowed) {
            /* UID not in allow list - deny access */
            if (s)
                __sync_fetch_and_add(&s->denied_count, 1);
            
            bpf_printk("DENIED: UID %u attempted to open blocked inode %llu\n", 
                      uid, ino);
            return -EPERM;
        }
    }

    if (s)
        __sync_fetch_and_add(&s->allowed_count, 1);

    return 0;  /* Allow */
}

/* LSM hook for program execution */
SEC("lsm/bprm_check_security")
int BPF_PROG(restrict_exec, struct linux_binprm *bprm, int ret)
{
    if (ret != 0)
        return ret;

    __u64 uid_gid = bpf_get_current_uid_gid();
    __u32 uid = uid_gid & 0xFFFFFFFF;

    /* Get inode of binary being executed - using pointer arithmetic */
    struct file *file = NULL;
    struct inode *inode = NULL;
    __u64 ino = 0;
    
    void *bprm_file_ptr = (char *)bprm + 0x18;  /* Approximate offset for file in linux_binprm */
    bpf_probe_read_kernel(&file, sizeof(file), bprm_file_ptr);
    if (file) {
        void *f_inode_ptr = (char *)file + 0x20;  /* Approximate offset for f_inode */
        bpf_probe_read_kernel(&inode, sizeof(inode), f_inode_ptr);
        if (inode) {
            void *i_ino_ptr = (char *)inode + 0x38;  /* Approximate offset for i_ino */
            bpf_probe_read_kernel(&ino, sizeof(ino), i_ino_ptr);
        }
    }

    __u32 *blocked = bpf_map_lookup_elem(&blocked_inodes, &ino);
    
    if (blocked && *blocked == 1) {
        __u32 *allowed = bpf_map_lookup_elem(&allowed_uids, &uid);
        if (!allowed) {
            bpf_printk("DENIED: UID %u attempted to execute blocked binary inode %llu\n",
                      uid, ino);
            return -EPERM;
        }
    }

    return 0;
}
