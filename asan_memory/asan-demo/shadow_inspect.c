#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __SANITIZE_ADDRESS__
// ASAN provides this function to check shadow memory
extern int __asan_address_is_poisoned(void const volatile *addr);
#else
int __asan_address_is_poisoned(void const volatile *addr) {
    (void)addr;
    return 0;  // Not using ASAN
}
#endif

void print_shadow_status(void *ptr, size_t size) {
    printf("\nShadow Memory Status:\n");
    printf("%-8s %-16s %s\n", "Offset", "Address", "Status");
    printf("------------------------------------------\n");
    
    for (size_t i = 0; i < size + 32; i++) {
        void *addr = (char*)ptr + i;
        int poisoned = __asan_address_is_poisoned(addr);
        
        printf("%-8zu %-16p %s\n", 
               i, 
               addr, 
               poisoned ? "\033[0;31mPOISONED\033[0m" : "\033[0;32mvalid\033[0m");
    }
}

int main() {
    printf("=== Shadow Memory Inspection ===\n\n");
    
    int *array = malloc(10 * sizeof(int));
    printf("Allocated 40 bytes (10 ints) at %p\n", (void*)array);
    
    print_shadow_status(array, 40);
    
    printf("\n--- Freeing memory ---\n");
    free(array);
    printf("Memory freed and placed in quarantine\n");
    
    print_shadow_status(array, 40);
    
    return 0;
}
