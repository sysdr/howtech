// SPDX-License-Identifier: GPL-2.0
/* Interactive Workload - Simulates low-latency request/response */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>

#define SLEEP_MS 10
#define WORK_ITERATIONS 5000

static inline unsigned long long rdtsc(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

static void do_work(void)
{
    unsigned long long start = rdtsc();
    volatile long sum = 0;
    
    for (int i = 0; i < WORK_ITERATIONS; i++) {
        sum += i * i;
    }
    
    unsigned long long end = rdtsc();
    (void)sum; /* Prevent optimization */
}

int main(int argc, char **argv)
{
    pid_t pid = syscall(SYS_gettid);
    
    printf("[Interactive] PID %d started (group 0 - low latency)\n", pid);
    printf("[Interactive] Pattern: sleep 10ms, work 50μs, repeat\n");
    
    unsigned long iterations = 0;
    
    while (1) {
        /* Sleep to simulate waiting for requests */
        usleep(SLEEP_MS * 1000);
        
        /* Do small amount of work (simulate request processing) */
        do_work();
        
        iterations++;
        
        if (iterations % 100 == 0) {
            printf("[Interactive] PID %d: %lu iterations completed\n", 
                   pid, iterations);
        }
    }
    
    return 0;
}
