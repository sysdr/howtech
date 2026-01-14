#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4
#define INCREMENTS 100000

atomic_llong counter = 0;

void* increment_counter(void* arg) {
    (void)arg; // Suppress unused parameter warning
    
    for (int i = 0; i < INCREMENTS; i++) {
        // Atomic increment - no race condition
        atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    pthread_t threads[NUM_THREADS];
    
    printf("Starting %d threads with atomic operations\n", NUM_THREADS);
    printf("Expected final value: %d\n\n", NUM_THREADS * INCREMENTS);
    
    for (long i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, (void*)i) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
    
    long long final_value = atomic_load(&counter);
    printf("Final counter value: %lld\n", final_value);
    
    return 0;
}
