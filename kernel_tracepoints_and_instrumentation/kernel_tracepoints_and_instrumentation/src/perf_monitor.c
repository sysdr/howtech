#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>

static volatile int keep_running = 1;

void sigint_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    keep_running = 0;
}

// Wrapper for perf_event_open syscall
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }
    
    pid_t target_pid = atoi(argv[1]);
    printf("Monitoring PID %d using perf_event_open()...\n\n", target_pid);
    
    signal(SIGINT, sigint_handler);
    
    // Set up perf events for the target process
    struct perf_event_attr pe_cycles, pe_instructions, pe_cache_misses;
    
    // CPU cycles
    memset(&pe_cycles, 0, sizeof(struct perf_event_attr));
    pe_cycles.type = PERF_TYPE_HARDWARE;
    pe_cycles.size = sizeof(struct perf_event_attr);
    pe_cycles.config = PERF_COUNT_HW_CPU_CYCLES;
    pe_cycles.disabled = 1;
    pe_cycles.exclude_kernel = 0;
    pe_cycles.exclude_hv = 1;
    
    // Instructions
    memset(&pe_instructions, 0, sizeof(struct perf_event_attr));
    pe_instructions.type = PERF_TYPE_HARDWARE;
    pe_instructions.size = sizeof(struct perf_event_attr);
    pe_instructions.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe_instructions.disabled = 1;
    pe_instructions.exclude_kernel = 0;
    pe_instructions.exclude_hv = 1;
    
    // Cache misses
    memset(&pe_cache_misses, 0, sizeof(struct perf_event_attr));
    pe_cache_misses.type = PERF_TYPE_HARDWARE;
    pe_cache_misses.size = sizeof(struct perf_event_attr);
    pe_cache_misses.config = PERF_COUNT_HW_CACHE_MISSES;
    pe_cache_misses.disabled = 1;
    pe_cache_misses.exclude_kernel = 0;
    pe_cache_misses.exclude_hv = 1;
    
    // Open perf events
    int fd_cycles = perf_event_open(&pe_cycles, target_pid, -1, -1, 0);
    int fd_instructions = perf_event_open(&pe_instructions, target_pid, -1, -1, 0);
    int fd_cache_misses = perf_event_open(&pe_cache_misses, target_pid, -1, -1, 0);
    
    if (fd_cycles < 0 || fd_instructions < 0 || fd_cache_misses < 0) {
        fprintf(stderr, "Error opening perf events (need CAP_PERFMON or root)\n");
        fprintf(stderr, "errno: %s\n", strerror(errno));
        return 1;
    }
    
    // Enable counting
    ioctl(fd_cycles, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_cache_misses, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_cycles, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_cache_misses, PERF_EVENT_IOC_ENABLE, 0);
    
    printf("%-20s %-20s %-20s %-10s\n", "CPU Cycles", "Instructions", "Cache Misses", "IPC");
    printf("%-20s %-20s %-20s %-10s\n", "----------", "------------", "------------", "---");
    
    uint64_t prev_cycles = 0, prev_instructions = 0, prev_cache_misses = 0;
    
    while (keep_running) {
        sleep(2);
        
        uint64_t cycles, instructions, cache_misses;
        
        if (read(fd_cycles, &cycles, sizeof(uint64_t)) < 0 ||
            read(fd_instructions, &instructions, sizeof(uint64_t)) < 0 ||
            read(fd_cache_misses, &cache_misses, sizeof(uint64_t)) < 0) {
            fprintf(stderr, "Process may have exited\n");
            break;
        }
        
        uint64_t delta_cycles = cycles - prev_cycles;
        uint64_t delta_instructions = instructions - prev_instructions;
        uint64_t delta_cache_misses = cache_misses - prev_cache_misses;
        
        double ipc = delta_cycles > 0 ? (double)delta_instructions / delta_cycles : 0.0;
        
        printf("%-20" PRIu64 " %-20" PRIu64 " %-20" PRIu64 " %-10.2f\n",
               delta_cycles, delta_instructions, delta_cache_misses, ipc);
        
        prev_cycles = cycles;
        prev_instructions = instructions;
        prev_cache_misses = cache_misses;
    }
    
    // Cleanup
    close(fd_cycles);
    close(fd_instructions);
    close(fd_cache_misses);
    
    printf("\nMonitoring stopped.\n");
    return 0;
}
