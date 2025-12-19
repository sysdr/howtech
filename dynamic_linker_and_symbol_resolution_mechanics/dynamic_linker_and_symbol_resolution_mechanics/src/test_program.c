#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_ALLOCATIONS 10

int main() {
    void* ptrs[NUM_ALLOCATIONS];
    
    printf("\n=== Starting Memory Allocation Test ===\n\n");
    
    // Allocate various sizes
    printf("Phase 1: Allocating memory...\n");
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        size_t size = (i + 1) * 1024; // 1KB, 2KB, 3KB, etc.
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            memset(ptrs[i], 'A' + i, size); // Write data
            printf("  Allocated %zu bytes at %p\n", size, ptrs[i]);
        }
        usleep(100000); // 100ms delay for visibility
    }
    
    printf("\nPhase 2: Using calloc...\n");
    void* zeroed = calloc(5, 2048);
    printf("  calloc(5, 2048) = %p\n", zeroed);
    
    printf("\nPhase 3: Freeing half the allocations...\n");
    for (int i = 0; i < NUM_ALLOCATIONS; i += 2) {
        printf("  Freeing %p\n", ptrs[i]);
        free(ptrs[i]);
        ptrs[i] = NULL;
        usleep(100000);
    }
    
    printf("\nPhase 4: Reallocating...\n");
    void* old_ptr = ptrs[1];
    void* realloced = realloc(ptrs[1], 8192);
    printf("  realloc(%p, 8192) = %p\n", old_ptr, realloced);
    if (realloced) {
        ptrs[1] = realloced;  // Update pointer if realloc succeeded
    } else {
        ptrs[1] = NULL;  // Mark as NULL if realloc failed
    }
    
    printf("\nPhase 5: Final cleanup...\n");
    for (int i = 1; i < NUM_ALLOCATIONS; i += 2) {
        if (ptrs[i]) {
            free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }
    free(zeroed);
    
    printf("\n=== Test Complete ===\n\n");
    
    return 0;
}
