#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdarg.h>
#include <x86intrin.h>

#define NUM_SYSCALLS 450
#define NUM_THREADS 4
#define ITERATIONS 10000

// Atomic counters for thread-safe syscall counting
static _Atomic uint64_t syscall_counts[NUM_SYSCALLS];
static _Atomic uint64_t syscall_cycles[NUM_SYSCALLS];

// Histogram buckets (logarithmic: 0-100, 100-200, 200-400, 400-800, etc.)
#define HIST_BUCKETS 16
static _Atomic uint64_t syscall_hist[NUM_SYSCALLS][HIST_BUCKETS];

// RDTSC timing with proper serialization
static inline uint64_t rdtsc_start(void) {
    uint64_t rax, rdx;
    
    // Serialize with CPUID
    asm volatile("cpuid" ::: "rax", "rbx", "rcx", "rdx");
    
    // Read TSC
    asm volatile("rdtsc" : "=a"(rax), "=d"(rdx));
    
    return (rdx << 32) | rax;
}

static inline uint64_t rdtsc_end(void) {
    uint32_t aux;
    uint64_t rax, rdx;
    
    // RDTSCP includes implicit serialization
    asm volatile("rdtscp" : "=a"(rax), "=d"(rdx), "=c"(aux));
    
    return (rdx << 32) | rax;
}

// Get histogram bucket for cycle count
static int get_bucket(uint64_t cycles) {
    if (cycles < 100) return 0;
    if (cycles < 200) return 1;
    if (cycles < 400) return 2;
    if (cycles < 800) return 3;
    if (cycles < 1600) return 4;
    if (cycles < 3200) return 5;
    if (cycles < 6400) return 6;
    if (cycles < 12800) return 7;
    if (cycles < 25600) return 8;
    if (cycles < 51200) return 9;
    if (cycles < 102400) return 10;
    if (cycles < 204800) return 11;
    if (cycles < 409600) return 12;
    if (cycles < 819200) return 13;
    if (cycles < 1638400) return 14;
    return 15;
}

// Record a syscall with timing
static void record_syscall(long nr, uint64_t cycles) {
    if (nr < 0 || nr >= NUM_SYSCALLS) return;
    
    atomic_fetch_add(&syscall_counts[nr], 1);
    atomic_fetch_add(&syscall_cycles[nr], cycles);
    
    int bucket = get_bucket(cycles);
    atomic_fetch_add(&syscall_hist[nr][bucket], 1);
}

// Instrumented syscall wrapper
static long instrumented_syscall(long nr, ...) {
    va_list args;
    long arg1, arg2, arg3, arg4, arg5, arg6;
    long ret;
    uint64_t start, end, cycles;
    
    va_start(args, nr);
    arg1 = va_arg(args, long);
    arg2 = va_arg(args, long);
    arg3 = va_arg(args, long);
    arg4 = va_arg(args, long);
    arg5 = va_arg(args, long);
    arg6 = va_arg(args, long);
    va_end(args);
    
    start = rdtsc_start();
    ret = syscall(nr, arg1, arg2, arg3, arg4, arg5, arg6);
    end = rdtsc_end();
    
    cycles = end - start;
    record_syscall(nr, cycles);
    
    return ret;
}

// Thread worker function
void* worker_thread(void* arg) {
    int tid = *(int*)arg;
    char buf[64];
    char filename[256];
    
    snprintf(filename, sizeof(filename), "/tmp/syscall_test_%d.tmp", tid);
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Test various syscalls
        instrumented_syscall(SYS_getpid);
        instrumented_syscall(SYS_gettid);
        
        int fd = (int)instrumented_syscall(SYS_openat, AT_FDCWD, filename,
                                           O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            instrumented_syscall(SYS_write, fd, buf, sizeof(buf));
            instrumented_syscall(SYS_close, fd);
        }
        
        // clock_gettime (may use VDSO)
        struct timespec ts;
        instrumented_syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
        
        // Small delay
        if (i % 100 == 0) {
            usleep(100);
        }
    }
    
    instrumented_syscall(SYS_unlinkat, AT_FDCWD, filename, 0);
    
    return NULL;
}

// Print syscall name
const char* syscall_name(long nr) {
    switch(nr) {
        case SYS_read: return "read";
        case SYS_write: return "write";
        case SYS_open: return "open";
        case SYS_close: return "close";
        case SYS_stat: return "stat";
        case SYS_fstat: return "fstat";
        case SYS_poll: return "poll";
        case SYS_lseek: return "lseek";
        case SYS_mmap: return "mmap";
        case SYS_mprotect: return "mprotect";
        case SYS_munmap: return "munmap";
        case SYS_brk: return "brk";
        case SYS_rt_sigaction: return "rt_sigaction";
        case SYS_rt_sigprocmask: return "rt_sigprocmask";
        case SYS_ioctl: return "ioctl";
        case SYS_pread64: return "pread64";
        case SYS_pwrite64: return "pwrite64";
        case SYS_readv: return "readv";
        case SYS_writev: return "writev";
        case SYS_access: return "access";
        case SYS_pipe: return "pipe";
        case SYS_select: return "select";
        case SYS_sched_yield: return "sched_yield";
        case SYS_mremap: return "mremap";
        case SYS_dup: return "dup";
        case SYS_dup2: return "dup2";
        case SYS_nanosleep: return "nanosleep";
        case SYS_getpid: return "getpid";
        case SYS_socket: return "socket";
        case SYS_connect: return "connect";
        case SYS_accept: return "accept";
        case SYS_sendto: return "sendto";
        case SYS_recvfrom: return "recvfrom";
        case SYS_bind: return "bind";
        case SYS_listen: return "listen";
        case SYS_getsockname: return "getsockname";
        case SYS_getpeername: return "getpeername";
        case SYS_clone: return "clone";
        case SYS_fork: return "fork";
        case SYS_vfork: return "vfork";
        case SYS_execve: return "execve";
        case SYS_exit: return "exit";
        case SYS_wait4: return "wait4";
        case SYS_kill: return "kill";
        case SYS_uname: return "uname";
        case SYS_fcntl: return "fcntl";
        case SYS_fsync: return "fsync";
        case SYS_fdatasync: return "fdatasync";
        case SYS_getdents: return "getdents";
        case SYS_getcwd: return "getcwd";
        case SYS_chdir: return "chdir";
        case SYS_rename: return "rename";
        case SYS_mkdir: return "mkdir";
        case SYS_rmdir: return "rmdir";
        case SYS_link: return "link";
        case SYS_unlink: return "unlink";
        case SYS_readlink: return "readlink";
        case SYS_chmod: return "chmod";
        case SYS_chown: return "chown";
        case SYS_gettimeofday: return "gettimeofday";
        case SYS_getrlimit: return "getrlimit";
        case SYS_getrusage: return "getrusage";
        case SYS_times: return "times";
        case SYS_getuid: return "getuid";
        case SYS_getgid: return "getgid";
        case SYS_geteuid: return "geteuid";
        case SYS_getegid: return "getegid";
        case SYS_getppid: return "getppid";
        case SYS_getpgrp: return "getpgrp";
        case SYS_setsid: return "setsid";
        case SYS_getsid: return "getsid";
        case SYS_getgroups: return "getgroups";
        case SYS_setpgid: return "setpgid";
        case SYS_prctl: return "prctl";
        case SYS_arch_prctl: return "arch_prctl";
        case SYS_futex: return "futex";
        case SYS_set_tid_address: return "set_tid_address";
        case SYS_clock_gettime: return "clock_gettime";
        case SYS_clock_getres: return "clock_getres";
        case SYS_clock_nanosleep: return "clock_nanosleep";
        case SYS_exit_group: return "exit_group";
        case SYS_epoll_create: return "epoll_create";
        case SYS_epoll_ctl: return "epoll_ctl";
        case SYS_epoll_wait: return "epoll_wait";
        case SYS_set_robust_list: return "set_robust_list";
        case SYS_get_robust_list: return "get_robust_list";
        case SYS_openat: return "openat";
        case SYS_mkdirat: return "mkdirat";
        case SYS_fchownat: return "fchownat";
        case SYS_futimesat: return "futimesat";
        case SYS_unlinkat: return "unlinkat";
        case SYS_renameat: return "renameat";
        case SYS_faccessat: return "faccessat";
        case SYS_pselect6: return "pselect6";
        case SYS_ppoll: return "ppoll";
        case SYS_splice: return "splice";
        case SYS_tee: return "tee";
        case SYS_sync_file_range: return "sync_file_range";
        case SYS_vmsplice: return "vmsplice";
        case SYS_utimensat: return "utimensat";
        case SYS_epoll_pwait: return "epoll_pwait";
        case SYS_fallocate: return "fallocate";
        case SYS_accept4: return "accept4";
        case SYS_dup3: return "dup3";
        case SYS_pipe2: return "pipe2";
        case SYS_preadv: return "preadv";
        case SYS_pwritev: return "pwritev";
        case SYS_recvmmsg: return "recvmmsg";
        case SYS_sendmmsg: return "sendmmsg";
        case SYS_gettid: return "gettid";
        default: return "unknown";
    }
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    printf("Starting syscall instrumentation test...\n");
    printf("Threads: %d, Iterations per thread: %d\n\n", NUM_THREADS, ITERATIONS);
    
    // Initialize counters
    memset((void*)syscall_counts, 0, sizeof(syscall_counts));
    memset((void*)syscall_cycles, 0, sizeof(syscall_cycles));
    memset((void*)syscall_hist, 0, sizeof(syscall_hist));
    
    // Create worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n=== Syscall Statistics ===\n\n");
    printf("%-20s %10s %12s %12s %12s\n",
           "Syscall", "Count", "Total Cycles", "Avg Cycles", "Min Bucket");
    printf("%-20s %10s %12s %12s %12s\n",
           "--------------------", "----------", "------------", "------------", "------------");
    
    // Print statistics for syscalls that were actually called
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        uint64_t count = atomic_load(&syscall_counts[i]);
        if (count > 0) {
            uint64_t cycles = atomic_load(&syscall_cycles[i]);
            uint64_t avg = cycles / count;
            
            // Find minimum non-zero histogram bucket
            int min_bucket = -1;
            for (int b = 0; b < HIST_BUCKETS; b++) {
                if (atomic_load(&syscall_hist[i][b]) > 0) {
                    min_bucket = b;
                    break;
                }
            }
            
            const char* min_str = min_bucket >= 0 ? 
                (min_bucket == 0 ? "<100" :
                 min_bucket == 1 ? "100-200" :
                 min_bucket == 2 ? "200-400" :
                 min_bucket == 3 ? "400-800" :
                 min_bucket == 4 ? "800-1.6k" : ">1.6k") : "N/A";
            
            printf("%-20s %10lu %12lu %12lu %12s\n",
                   syscall_name(i), count, cycles, avg, min_str);
        }
    }
    
    printf("\n=== Detailed Histogram (top 3 syscalls) ===\n\n");
    
    // Find top 3 syscalls by count
    typedef struct { long nr; uint64_t count; } top_entry_t;
    top_entry_t top[3] = {{-1, 0}, {-1, 0}, {-1, 0}};
    
    for (int i = 0; i < NUM_SYSCALLS; i++) {
        uint64_t count = atomic_load(&syscall_counts[i]);
        if (count > top[2].count) {
            top[2].nr = i;
            top[2].count = count;
            // Bubble sort
            for (int j = 2; j > 0 && top[j].count > top[j-1].count; j--) {
                top_entry_t temp = top[j];
                top[j] = top[j-1];
                top[j-1] = temp;
            }
        }
    }
    
    const char* bucket_names[] = {
        "0-100", "100-200", "200-400", "400-800",
        "800-1.6k", "1.6k-3.2k", "3.2k-6.4k", "6.4k-12.8k",
        "12.8k-25.6k", "25.6k-51.2k", "51.2k-102k", "102k-204k",
        "204k-409k", "409k-819k", "819k-1.6M", ">1.6M"
    };
    
    for (int i = 0; i < 3 && top[i].nr >= 0; i++) {
        long nr = top[i].nr;
        printf("%s (count: %lu):\n", syscall_name(nr), top[i].count);
        
        for (int b = 0; b < HIST_BUCKETS; b++) {
            uint64_t bucket_count = atomic_load(&syscall_hist[nr][b]);
            if (bucket_count > 0) {
                double pct = 100.0 * bucket_count / top[i].count;
                printf("  %12s: %8lu (%5.1f%%) ", bucket_names[b], bucket_count, pct);
                
                // Simple bar chart
                int bars = (int)(pct / 2);
                for (int j = 0; j < bars && j < 50; j++) printf("█");
                printf("\n");
            }
        }
        printf("\n");
    }
    
    printf("Test completed successfully.\n");
    return 0;
}

