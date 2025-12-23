#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>

// These will be resolved through PLT/GOT
extern int expensive_calculation(int n);
extern void print_message(const char* msg);

// Function that triggers lazy binding on first call
void trigger_lazy_binding() {
    printf("Calling external functions (triggers PLT resolution)...\n");
    expensive_calculation(42);
    print_message("Hello from dynamically linked library!");
}

// Demonstrate runtime linking with dlopen
void runtime_linking_demo() {
    printf("\n=== Runtime Dynamic Linking (dlopen) ===\n");
    
    void* handle = dlopen("./build/libruntime.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return;
    }
    
    // Look up symbol at runtime
    void (*runtime_func)(void) = dlsym(handle, "runtime_loaded_function");
    if (runtime_func) {
        runtime_func();
    }
    
    dlclose(handle);
}

int main(int argc, char* argv[]) {
    printf("=== ELF Binary Loading Demo ===\n");
    printf("PID: %d\n", getpid());
    printf("Program started. Dynamic linker has loaded shared libraries.\n\n");
    
    // Prevent unused-parameter warning when building with -Werror
    (void)argv;
    
    // Show that we're position-independent
    printf("Main function address: %p\n", (void*)main);
    printf("Stack address: %p\n", (void*)&argc);
    printf("Heap address: %p\n\n", malloc(1));
    
    printf("=== Lazy Binding Demonstration ===\n");
    printf("About to call external function for the FIRST time.\n");
    printf("Watch for PLT -> GOT -> _dl_runtime_resolve pattern.\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    trigger_lazy_binding();  // First call - triggers resolution
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    long first_call = (end.tv_sec - start.tv_sec) * 1000000000L + 
                      (end.tv_nsec - start.tv_nsec);
    
    printf("\nCalling same functions AGAIN (should use resolved GOT entries)...\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    trigger_lazy_binding();  // Second call - uses cached resolution
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long second_call = (end.tv_sec - start.tv_sec) * 1000000000L + 
                       (end.tv_nsec - start.tv_nsec);
    
    printf("\nTiming comparison:\n");
    printf("  First call (with lazy binding):  %ld ns\n", first_call);
    printf("  Second call (GOT already set):   %ld ns\n", second_call);
    printf("  Speedup: %.2fx\n", (double)first_call / second_call);
    
    runtime_linking_demo();
    
    printf("\n=== Check /proc/%d/maps to see loaded libraries ===\n", getpid());
    printf("Press Enter to continue...");
    getchar();
    
    return 0;
}
