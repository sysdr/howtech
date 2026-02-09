/* Fallback vmlinux.h - Basic type definitions for BPF compilation */
#ifndef __VMLINUX_H__
#define __VMLINUX_H__

/* Standard integer types */
typedef unsigned char __u8;
typedef signed char __s8;
typedef short int __s16;
typedef short unsigned int __u16;
typedef int __s32;
typedef unsigned int __u32;
typedef long long int __s64;
typedef long long unsigned int __u64;

/* BPF-friendly type aliases */
typedef __u8 u8;
typedef __s8 s8;
typedef __s16 s16;
typedef __u16 u16;
typedef __s32 s32;
typedef __u32 u32;
typedef __s64 s64;
typedef __u64 u64;

/* Network-related types (needed by BPF helpers) */
typedef __u16 __be16;
typedef __u32 __be32;
typedef __u32 __wsum;

/* Forward declarations */
struct task_struct {
	__u32 pid;
	int static_prio;
	void *cpus_ptr;
};

struct scx_exit_info {
	char *msg;
	int kind;
};

#define SCX_DSQ_GLOBAL 0xffffffffffffffffULL

#endif /* __VMLINUX_H__ */
