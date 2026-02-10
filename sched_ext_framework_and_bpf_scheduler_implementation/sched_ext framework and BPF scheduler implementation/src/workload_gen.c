// SPDX-License-Identifier: GPL-2.0
/* Generates mixed workload to demonstrate scheduler behavior */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define NUM_HIGH_PRIO 2
#define NUM_MEDIUM_PRIO 3
#define NUM_LOW_PRIO 5

static void cpu_work(unsigned long iterations)
{
    volatile unsigned long sum = 0;
    for (unsigned long i = 0; i < iterations; i++) {
        sum += i * i;
    }
}

static void worker(const char *name, int nice_value, unsigned long work_amount)
{
    if (setpriority(PRIO_PROCESS, 0, nice_value) < 0) {
        fprintf(stderr, "%s: Failed to set priority: %s\n", name, strerror(errno));
    }
    
    printf("[%s] Started with nice=%d, PID=%d\n", name, nice_value, getpid());
    
    for (int i = 0; i < 10; i++) {
        cpu_work(work_amount);
        usleep(100000); /* 100ms */
    }
    
    printf("[%s] Completed\n", name);
    exit(0);
}

int main(int argc, char **argv)
{
    pid_t pids[NUM_HIGH_PRIO + NUM_MEDIUM_PRIO + NUM_LOW_PRIO];
    int pid_count = 0;
    
    printf("=== Multi-Level Scheduler Workload Generator ===\n\n");
    
    /* Spawn high-priority workers (nice -10) */
    printf("Spawning %d HIGH priority workers (nice -10)...\n", NUM_HIGH_PRIO);
    for (int i = 0; i < NUM_HIGH_PRIO; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char name[32];
            snprintf(name, sizeof(name), "HIGH-%d", i);
            worker(name, -10, 1000000);
        } else if (pid > 0) {
            pids[pid_count++] = pid;
        }
    }
    
    /* Spawn medium-priority workers (nice 5) */
    printf("Spawning %d MEDIUM priority workers (nice 5)...\n", NUM_MEDIUM_PRIO);
    for (int i = 0; i < NUM_MEDIUM_PRIO; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char name[32];
            snprintf(name, sizeof(name), "MEDIUM-%d", i);
            worker(name, 5, 500000);
        } else if (pid > 0) {
            pids[pid_count++] = pid;
        }
    }
    
    /* Spawn low-priority workers (nice 15) */
    printf("Spawning %d LOW priority workers (nice 15)...\n", NUM_LOW_PRIO);
    for (int i = 0; i < NUM_LOW_PRIO; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char name[32];
            snprintf(name, sizeof(name), "LOW-%d", i);
            worker(name, 15, 250000);
        } else if (pid > 0) {
            pids[pid_count++] = pid;
        }
    }
    
    printf("\nAll workers spawned. Waiting for completion...\n\n");
    
    /* Wait for all workers */
    for (int i = 0; i < pid_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    printf("\n=== All workers completed ===\n");
    return 0;
}
