#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define NUM_THREADS 4
#define ITERATIONS 10000000
#define CACHE_LINE_SIZE 64

// Aligned to cache line to prevent false sharing of the lock itself
typedef struct {
    atomic_flag lock;
    char padding[CACHE_LINE_SIZE - sizeof(atomic_flag)];
} aligned_spinlock_t;

// Counter on its own cache line
typedef struct {
    volatile long counter;
    char padding[CACHE_LINE_SIZE - sizeof(long)];
} aligned_counter_t;

static aligned_spinlock_t spinlock = {.lock = ATOMIC_FLAG_INIT, .padding = {0}};
static aligned_counter_t global_counter = {.counter = 0, .padding = {0}};
static volatile int running = 1;

// Statistics per thread
typedef struct {
    unsigned long acquisitions;
    unsigned long spins;
    unsigned long cache_misses;  // Approximation
} thread_stats_t;

static thread_stats_t thread_stats[NUM_THREADS];

static inline unsigned long long rdtsc(void) {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
}

void spinlock_acquire(int thread_id) {
    unsigned long spins = 0;
    while (atomic_flag_test_and_set_explicit(&spinlock.lock, memory_order_acquire)) {
        spins++;
        // CPU pause to reduce contention
        __asm__ __volatile__("pause" ::: "memory");
    }
    thread_stats[thread_id].spins += spins;
    thread_stats[thread_id].acquisitions++;
}

void spinlock_release(void) {
    atomic_flag_clear_explicit(&spinlock.lock, memory_order_release);
}

void* worker_thread(void* arg) {
    long thread_id = (long)arg;
    cpu_set_t cpuset;
    
    // Pin thread to specific CPU
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Warning: Failed to set CPU affinity for thread %ld\n", thread_id);
    }
    
    unsigned long long start_cycles = rdtsc();
    
    for (int i = 0; i < ITERATIONS; i++) {
        spinlock_acquire(thread_id);
        
        // Critical section - very short, simulating hot counter
        global_counter.counter++;
        
        spinlock_release();
    }
    
    unsigned long long end_cycles = rdtsc();
    unsigned long long elapsed = end_cycles - start_cycles;
    
    printf("[Thread %ld] Completed: %lu acquisitions, %lu spins, ~%.2f cycles/acquisition\n",
           thread_id, 
           thread_stats[thread_id].acquisitions,
           thread_stats[thread_id].spins,
           (double)elapsed / ITERATIONS);
    
    return NULL;
}

void print_stats(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Spinlock Contention Statistics\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Thread  | Acquisitions | Total Spins | Avg Spins/Acq\n");
    printf("────────┼──────────────┼─────────────┼──────────────\n");
    
    unsigned long total_spins = 0;
    unsigned long total_acq = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("  %2d    | %12lu | %11lu | %12.2f\n",
               i,
               thread_stats[i].acquisitions,
               thread_stats[i].spins,
               (double)thread_stats[i].spins / thread_stats[i].acquisitions);
        total_spins += thread_stats[i].spins;
        total_acq += thread_stats[i].acquisitions;
    }
    
    printf("────────┼──────────────┼─────────────┼──────────────\n");
    printf("Total   | %12lu | %11lu | %12.2f\n",
           total_acq, total_spins, (double)total_spins / total_acq);
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Final counter value: %ld (expected: %d)\n", 
           global_counter.counter, NUM_THREADS * ITERATIONS);
    printf("═══════════════════════════════════════════════════════════\n");
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    
    printf("Starting %d threads, each performing %d lock acquisitions...\n", 
           NUM_THREADS, ITERATIONS);
    printf("Watch for high spin counts indicating cache line bouncing\n\n");
    
    // Create threads
    for (long i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, (void*)i) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    // Wait for completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    print_stats();
    
    return 0;
}
