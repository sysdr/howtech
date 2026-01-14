#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS 4
#define INCREMENTS 100000

// Shared counter - INTENTIONALLY NOT PROTECTED
volatile long long counter = 0;

void* increment_counter(void* arg) {
    (void)arg; // Suppress unused parameter warning
    
    for (int i = 0; i < INCREMENTS; i++) {
        // This is NOT atomic - race condition here!
        // counter++ is actually: temp = counter; temp++; counter = temp;
        counter++;
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    pthread_t threads[NUM_THREADS];
    
    printf("Starting %d threads, each incrementing counter %d times\n", 
           NUM_THREADS, INCREMENTS);
    printf("Expected final value: %d\n\n", NUM_THREADS * INCREMENTS);
    
    // Create threads
    for (long i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, increment_counter, (void*)i) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    // Wait for all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }
    
    printf("Final counter value: %lld\n", counter);
    printf("Lost updates: %d\n", (NUM_THREADS * INCREMENTS) - (int)counter);
    
    return 0;
}
