#include <stdio.h>
#include <dlfcn.h>

// External declarations
extern void increment_global(void);
extern int get_global(void);
extern int weak_function(void);

// Our own weak symbol - will override library's if linked
__attribute__((weak)) int weak_function(void) {
    return 999;  // Different from library's 42
}

int main(int argc, char** argv) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    printf("=== Symbol Resolution Demo ===\n\n");
    
    // Use library functions
    printf("Initial global counter: %d\n", get_global());
    increment_global();
    printf("After increment: %d\n", get_global());
    
    // Call weak function - which one gets called?
    printf("Weak function returns: %d\n", weak_function());
    
    // Dynamic loading demonstration
    printf("\n=== Dynamic Symbol Loading ===\n");
    void* handle = dlopen("./build/libmylib.so", RTLD_LAZY);
    if (handle) {
        // Try to load a symbol
        int (*get_fn)(void) = dlsym(handle, "get_global");
        if (get_fn) {
            printf("Dynamically loaded function returned: %d\n", get_fn());
        }
        
        // Try to load hidden symbol - will fail
        void* hidden = dlsym(handle, "hidden_function");
        if (!hidden) {
            printf("Hidden symbol not accessible: %s\n", dlerror());
        }
        
        dlclose(handle);
    }
    
    return 0;
}
