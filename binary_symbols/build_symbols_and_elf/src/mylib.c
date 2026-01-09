#include <stdio.h>
#include <stdlib.h>

// Global strong symbol
int global_counter = 0;

// Local symbol (static)
static int local_counter = 0;

// Weak symbol - can be overridden
__attribute__((weak)) int weak_function(void) {
    return 42;
}

// Hidden symbol - not exported
__attribute__((visibility("hidden"))) void hidden_function(void) {
    printf("Hidden function called\n");
}

// Normal exported function
void increment_global(void) {
    global_counter++;
    local_counter++;
}

int get_global(void) {
    return global_counter;
}

// COMMON symbol (uninitialized global) - will appear as 'B' or 'C'
int uninitialized_data;

// Thread-local storage
__thread int tls_variable = 0;

// GNU indirect function (IFUNC) - runtime dispatch
static int (*resolve_optimized(void))(void) {
    return weak_function;
}

int optimized_function(void) __attribute__((ifunc("resolve_optimized")));

// Constructor - runs before main
__attribute__((constructor)) void lib_init(void) {
    global_counter = 100;
}

// Destructor - runs after main
__attribute__((destructor)) void lib_cleanup(void) {
    // Cleanup code
}
