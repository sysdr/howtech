#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4
#define INCREMENTS 100000

long long counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void* increment_counter(void* arg) {
    (void)arg; // Suppress unused parameter warning
    
    for (int i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&counter_mutex);
        counter++;
        pthread_mutex_unlock(&counter_mutex);
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    pthread_t threads[NUM_THREADS];
    
    printf("Starting %d threads with mutex protection\n", NUM_THREADS);
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
    
    printf("Final counter value: %lld\n", counter);
    
    pthread_mutex_destroy(&counter_mutex);
    return 0;
}
