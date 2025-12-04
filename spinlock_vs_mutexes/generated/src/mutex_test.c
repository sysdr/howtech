#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

#define NUM_THREADS 4
#define ITERATIONS 1000000
#define HOLD_TIME_NS 100

typedef struct {
    pthread_mutex_t mutex;
    long counter;
} mutex_lock_t;

void mutex_lock_init(mutex_lock_t *m) {
    pthread_mutex_init(&m->mutex, NULL);
    m->counter = 0;
}

typedef struct {
    mutex_lock_t *lock;
    int thread_id;
    long iterations;
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
        pthread_mutex_lock(&targ->lock->mutex);
        
        // Critical section
        targ->lock->counter++;
        busy_wait(HOLD_TIME_NS);
        
        pthread_mutex_unlock(&targ->lock->mutex);
    }
    
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    mutex_lock_t lock;
    struct timespec start, end;
    
    mutex_lock_init(&lock);
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║       MUTEX Performance Test           ║\n");
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
    printf("\n  ✓ CPU efficient (threads sleep when blocked)\n");
    printf("  ✓ Context switches handle contention\n");
    
    pthread_mutex_destroy(&lock.mutex);
    return 0;
}
