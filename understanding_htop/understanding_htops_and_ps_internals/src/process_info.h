#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <sys/types.h>

struct process_info {
    pid_t pid;
    pid_t ppid;
    char comm[256];
    char state;
    int num_threads;
    unsigned long utime;       /* CPU time in user mode (clock ticks) */
    unsigned long stime;       /* CPU time in kernel mode (clock ticks) */
    unsigned long vsize;       /* Virtual memory size (bytes) */
    unsigned long rss;         /* Resident set size (bytes) */
    unsigned long rss_anon;    /* Anonymous RSS (bytes) */
    unsigned long rss_file;    /* File-backed RSS (bytes) */
    unsigned long rss_shmem;   /* Shared memory RSS (bytes) */
};

void init_system_info(void);
int parse_proc_stat(pid_t pid, struct process_info *info);
int parse_proc_status(pid_t pid, struct process_info *info);
int get_fd_count(pid_t pid);
void calculate_cpu_usage(struct process_info *prev, struct process_info *curr,
                         unsigned long elapsed_ticks, int num_cpus,
                         double *user_pct, double *sys_pct);
int get_process_list(pid_t **pids, int *count);
long get_clock_ticks(void);
int get_num_cpus(void);

#endif
