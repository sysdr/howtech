#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

#define NUM_THREADS 4
#define ITERATIONS 1000000
#define HOLD_TIME_NS 100  // Critical section hold time

typedef struct {
    atomic_int lock;
    long counter;
} spinlock_t;

void spinlock_init(spinlock_t *s) {
    atomic_init(&s->lock, 0);
    s->counter = 0;
}

void spinlock_acquire(spinlock_t *s) {
    int expected;
    do {
        expected = 0;
    } while (!atomic_compare_exchange_weak(&s->lock, &expected, 1));
}

void spinlock_release(spinlock_t *s) {
    atomic_store(&s->lock, 0);
}

typedef struct {
    spinlock_t *lock;
    int thread_id;
    long iterations;
    long spins;
    struct timespec start_time;
} thread_arg_t;

void busy_wait(long nanoseconds) {
    struct timespec start, current;
    clock_gettime(CLOCK_MONOTONIC, &start);
    do {
        clock_gettime(CLOCK_MONOTONIC, &current);
    } while ((current.tv_sec - start.tv_sec) * 1000000000L + 
             (current.tv_nsec - start.tv_nsec) < nanoseconds);
}

void* worker_thread(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    
    // Pin to specific CPU
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(targ->thread_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    for (long i = 0; i < targ->iterations; i++) {
        spinlock_acquire(targ->lock);
        
        // Critical section - simulate work
        targ->lock->counter++;
        busy_wait(HOLD_TIME_NS);
        
        spinlock_release(targ->lock);
        
        targ->spins++;
    }
    
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    spinlock_t lock;
    struct timespec start, end;
    
    spinlock_init(&lock);
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║      SPINLOCK Performance Test         ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf("Configuration:\n");
    printf("  Threads:              %d\n", NUM_THREADS);
    printf("  Iterations/thread:    %d\n", ITERATIONS);
    printf("  Hold time:            %dns\n", HOLD_TIME_NS);
    printf("  Total operations:     %d\n\n", NUM_THREADS * ITERATIONS);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].lock = &lock;
        args[i].thread_id = i;
        args[i].iterations = ITERATIONS;
        args[i].spins = 0;
        args[i].start_time = start;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &args[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    double ops_per_sec = (NUM_THREADS * ITERATIONS) / elapsed;
    double avg_latency_ns = (elapsed * 1e9) / (NUM_THREADS * ITERATIONS);
    
    printf("Results:\n");
    printf("  Final counter:        %ld (expected: %d)\n", 
           lock.counter, NUM_THREADS * ITERATIONS);
    printf("  Total time:           %.3f seconds\n", elapsed);
    printf("  Operations/second:    %.0f ops/sec\n", ops_per_sec);
    printf("  Avg latency:          %.1f ns/op\n", avg_latency_ns);
    printf("\n  ⚠  CPU usage was 100%% (busy-waiting)\n");
    printf("  ⚠  Cache line bouncing between cores\n");
    
    return 0;
}
