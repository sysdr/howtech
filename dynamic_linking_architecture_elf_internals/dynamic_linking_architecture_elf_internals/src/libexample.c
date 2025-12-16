#include <stdio.h>
#include <unistd.h>
#include <time.h>

// Simple function that will be called via PLT
void example_function(const char *msg) {
    printf("[libexample.so] %s\n", msg);
}

// Another function to show multiple GOT entries
void another_function(int value) {
    printf("[libexample.so] Got value: %d\n", value);
}

// Function that calls libc functions (nested PLT calls)
void complex_function(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("[libexample.so] Time: %ld.%09ld\n", ts.tv_sec, ts.tv_nsec);
}
