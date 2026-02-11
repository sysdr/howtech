// SPDX-License-Identifier: GPL-2.0
/* Batch Workload - Simulates CPU-intensive throughput task */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#define WORK_ITERATIONS 1000000

static void do_compute(void)
{
    volatile double result = 1.0;
    
    for (int i = 1; i < WORK_ITERATIONS; i++) {
        result += 1.0 / (i * i);
    }
    
    (void)result;
}

int main(int argc, char **argv)
{
    pid_t pid = syscall(SYS_gettid);
    
    printf("[Batch] PID %d started (group 1 - throughput)\n", pid);
    printf("[Batch] Pattern: continuous computation\n");
    
    unsigned long iterations = 0;
    
    while (1) {
        do_compute();
        
        iterations++;
        
        if (iterations % 10 == 0) {
            printf("[Batch] PID %d: %lu iterations completed\n", 
                   pid, iterations);
        }
    }
    
    return 0;
}
