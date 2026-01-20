#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 8
#define READER_DURATION_SEC 3

static atomic_int readers_active = 0;
static atomic_int grace_period_waiting = 0;

void* reader_thread(void* arg) {
    long id = (long)arg;
    
    for (int i = 0; i < 5; i++) {
        // Simulate rcu_read_lock()
        atomic_fetch_add(&readers_active, 1);
        printf("[Reader %ld] Entered RCU critical section (active readers: %d)\n", 
               id, atomic_load(&readers_active));
        
        // Long-running read operation
        sleep(READER_DURATION_SEC);
        
        // Simulate rcu_read_unlock()
        atomic_fetch_sub(&readers_active, 1);
        printf("[Reader %ld] Exited RCU critical section\n", id);
        
        // Small delay before re-entering
        usleep(500000);  // 500ms
    }
    
    return NULL;
}

void* writer_thread(void* arg) {
    (void)arg;  // Unused parameter
    sleep(1);  // Let some readers start
    
    printf("\n[Writer] Calling synchronize_rcu() - waiting for grace period...\n");
    atomic_store(&grace_period_waiting, 1);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Wait for all readers to exit (simulating synchronize_rcu)
    int warned = 0;
    while (atomic_load(&readers_active) > 0) {
        int active = atomic_load(&readers_active);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (elapsed > 2.0 && !warned) {
            printf("\n⚠ WARNING: Grace period stall! Still %d readers active after %.1fs\n",
                   active, elapsed);
            printf("   This simulates: INFO: rcu_sched detected stalls on CPUs\n");
            warned = 1;
        }
        
        usleep(100000);  // 100ms
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double total_time = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("\n[Writer] Grace period completed after %.2f seconds\n", total_time);
    printf("[Writer] This is a live lock: readers made progress, writer didn't\n");
    
    return NULL;
}

int main(void) {
    pthread_t readers[NUM_READERS];
    pthread_t writer;
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RCU Grace Period Stall Demonstration\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Starting %d reader threads with overlapping critical sections\n", NUM_READERS);
    printf("Writer will block waiting for quiescent state\n\n");
    
    // Create readers
    for (long i = 0; i < NUM_READERS; i++) {
        pthread_create(&readers[i], NULL, reader_thread, (void*)i);
        usleep(300000);  // Stagger starts
    }
    
    // Create writer
    pthread_create(&writer, NULL, writer_thread, NULL);
    
    // Wait for completion
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    pthread_join(writer, NULL);
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    
    return 0;
}
