#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>

#define HIGH_PRIO    90
#define MEDIUM_PRIO  50
#define LOW_PRIO     10

/* Shared data structure */
typedef struct {
    pthread_mutex_t lock;
    atomic_int high_blocked_count;
    atomic_int high_acquired_count;
    atomic_int medium_preempt_count;
    atomic_long high_wait_time_ns;
    int use_pi;
    volatile int running;
} shared_data_t;

shared_data_t shared;

/* Get current time in nanoseconds */
static inline long get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/* Set thread to real-time scheduling */
static int set_realtime_priority(int priority) {
    struct sched_param param;
    param.sched_priority = priority;
    
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        if (errno == EPERM) {
            fprintf(stderr, "Warning: Need CAP_SYS_NICE or root for SCHED_FIFO\n");
            fprintf(stderr, "Run with: sudo ./priority_inversion <use_pi>\n");
            return -1;
        }
        perror("sched_setscheduler");
        return -1;
    }
    return 0;
}

/* Low priority task - holds lock for extended period */
void* low_priority_task(void* arg) {
    (void)arg; // Unused parameter
    printf("[LOW] Starting (priority %d)\n", LOW_PRIO);
    
    if (set_realtime_priority(LOW_PRIO) < 0) {
        return NULL;
    }
    
    while (shared.running) {
        pthread_mutex_lock(&shared.lock);
        
        /* Simulate some work while holding the lock */
        struct timespec sleep_time = {0, 50000000}; // 50ms
        nanosleep(&sleep_time, NULL);
        
        pthread_mutex_unlock(&shared.lock);
        
        usleep(100000); // 100ms between acquisitions
    }
    
    printf("[LOW] Exiting\n");
    return NULL;
}

/* Medium priority task - CPU intensive, doesn't need lock */
void* medium_priority_task(void* arg) {
    (void)arg; // Unused parameter
    printf("[MEDIUM] Starting (priority %d)\n", MEDIUM_PRIO);
    
    if (set_realtime_priority(MEDIUM_PRIO) < 0) {
        return NULL;
    }
    
    while (shared.running) {
        /* Simulate CPU-intensive work */
        volatile long sum = 0;
        for (int i = 0; i < 1000000; i++) {
            sum += i;
        }
        
        atomic_fetch_add(&shared.medium_preempt_count, 1);
        usleep(10000); // 10ms
    }
    
    printf("[MEDIUM] Exiting\n");
    return NULL;
}

/* High priority task - needs lock frequently */
void* high_priority_task(void* arg) {
    (void)arg; // Unused parameter
    printf("[HIGH] Starting (priority %d)\n", HIGH_PRIO);
    
    if (set_realtime_priority(HIGH_PRIO) < 0) {
        return NULL;
    }
    
    usleep(25000); // Let low priority task acquire lock first
    
    while (shared.running) {
        long start_time = get_time_ns();
        
        atomic_fetch_add(&shared.high_blocked_count, 1);
        pthread_mutex_lock(&shared.lock);
        
        long wait_time = get_time_ns() - start_time;
        atomic_fetch_add(&shared.high_wait_time_ns, wait_time);
        atomic_fetch_add(&shared.high_acquired_count, 1);
        
        /* Quick work with the lock */
        struct timespec sleep_time = {0, 1000000}; // 1ms
        nanosleep(&sleep_time, NULL);
        
        pthread_mutex_unlock(&shared.lock);
        
        usleep(20000); // 20ms between requests
    }
    
    printf("[HIGH] Exiting\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t low_thread, medium_thread, high_thread;
    pthread_mutexattr_t attr;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <use_pi>\n", argv[0]);
        fprintf(stderr, "  use_pi: 0 = regular mutex, 1 = PI mutex\n");
        return 1;
    }
    
    shared.use_pi = atoi(argv[1]);
    shared.running = 1;
    atomic_init(&shared.high_blocked_count, 0);
    atomic_init(&shared.high_acquired_count, 0);
    atomic_init(&shared.medium_preempt_count, 0);
    atomic_init(&shared.high_wait_time_ns, 0);
    
    /* Initialize mutex with or without PI */
    pthread_mutexattr_init(&attr);
    if (shared.use_pi) {
        printf("=== Using Priority Inheritance (PI) Mutex ===\n");
        if (pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT) != 0) {
            perror("pthread_mutexattr_setprotocol");
            return 1;
        }
    } else {
        printf("=== Using Regular Mutex (No PI) ===\n");
    }
    
    pthread_mutex_init(&shared.lock, &attr);
    pthread_mutexattr_destroy(&attr);
    
    /* Create threads in specific order */
    if (pthread_create(&low_thread, NULL, low_priority_task, NULL) != 0) {
        perror("pthread_create low");
        return 1;
    }
    
    usleep(10000); // Ensure low priority starts first
    
    if (pthread_create(&medium_thread, NULL, medium_priority_task, NULL) != 0) {
        perror("pthread_create medium");
        return 1;
    }
    
    if (pthread_create(&high_thread, NULL, high_priority_task, NULL) != 0) {
        perror("pthread_create high");
        return 1;
    }
    
    /* Run for 3 seconds */
    sleep(3);
    
    shared.running = 0;
    
    /* Wait for threads */
    pthread_join(high_thread, NULL);
    pthread_join(medium_thread, NULL);
    pthread_join(low_thread, NULL);
    
    pthread_mutex_destroy(&shared.lock);
    
    /* Print statistics */
    printf("\n=== Results ===\n");
    printf("High priority task:\n");
    printf("  Lock requests: %d\n", atomic_load(&shared.high_blocked_count));
    printf("  Lock acquired: %d\n", atomic_load(&shared.high_acquired_count));
    
    int requests = atomic_load(&shared.high_blocked_count);
    if (requests > 0) {
        long total_wait = atomic_load(&shared.high_wait_time_ns);
        long avg_wait = total_wait / requests;
        printf("  Average wait time: %ld.%03ld ms\n", 
               avg_wait / 1000000, (avg_wait / 1000) % 1000);
        printf("  Total wait time: %ld.%03ld ms\n",
               total_wait / 1000000, (total_wait / 1000) % 1000);
    }
    
    printf("Medium priority CPU cycles: %d\n", atomic_load(&shared.medium_preempt_count));
    
    printf("\nInterpretation:\n");
    if (shared.use_pi) {
        printf("With PI: Low priority task is boosted when high priority waits.\n");
        printf("Medium priority cannot preempt the boosted low priority task.\n");
        printf("Result: Lower wait times for high priority task.\n");
    } else {
        printf("Without PI: Medium priority preempts low priority task.\n");
        printf("High priority task waits for low to complete (priority inversion).\n");
        printf("Result: Higher wait times for high priority task.\n");
    }
    
    return 0;
}
