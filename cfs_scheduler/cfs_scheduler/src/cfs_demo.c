#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sched.h>

#define NUM_TASKS 5
#define RUNTIME_SEC 10

typedef struct {
    pid_t pid;
    int nice_value;
    char name[32];
} task_info_t;

void cpu_intensive_work(void) {
    volatile unsigned long counter = 0;
    for (int i = 0; i < 50000000; i++) {
        counter += i;
    }
}

void run_task(const char *name, int nice_val) {
    if (nice(nice_val) == -1 && errno != 0) {
        fprintf(stderr, "Warning: nice(%d) failed for %s: %s\n", 
                nice_val, name, strerror(errno));
    }
    
    printf("[%s] Starting with nice=%d, PID=%d\n", name, nice_val, getpid());
    fflush(stdout);
    
    time_t start = time(NULL);
    unsigned long iterations = 0;
    
    while (time(NULL) - start < RUNTIME_SEC) {
        cpu_intensive_work();
        iterations++;
    }
    
    printf("[%s] Completed %lu iterations\n", name, iterations);
    exit(0);
}

int main(void) {
    printf("CFS Scheduler Demonstration\n");
    printf("===========================\n");
    printf("Creating %d tasks with different nice values...\n\n", NUM_TASKS);
    
    task_info_t tasks[NUM_TASKS] = {
        {0, -5, "HIGH_PRIO"},
        {0,  0, "NORMAL_1"},
        {0,  0, "NORMAL_2"},
        {0,  5, "LOW_PRIO"},
        {0, 10, "VERY_LOW"}
    };
    
    // Fork child processes
    for (int i = 0; i < NUM_TASKS; i++) {
        pid_t pid = fork();
        
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (pid == 0) {
            // Child process
            run_task(tasks[i].name, tasks[i].nice_value);
            // Never returns
        }
        
        // Parent process
        tasks[i].pid = pid;
        usleep(10000); // Small delay between forks
    }
    
    printf("\nAll tasks started. They will run for %d seconds...\n", RUNTIME_SEC);
    printf("Monitor their vruntime in real-time with: ./build/cfs_monitor\n\n");
    
    // Wait for all children
    for (int i = 0; i < NUM_TASKS; i++) {
        int status;
        pid_t pid = waitpid(tasks[i].pid, &status, 0);
        if (pid == -1) {
            perror("waitpid");
        }
    }
    
    printf("\n=== All tasks completed ===\n");
    printf("Check output/vruntime_log.txt for detailed timing analysis\n");
    
    return 0;
}
