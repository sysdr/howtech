#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Double Free Test ===\n");
    
    int *ptr = malloc(100);
    printf("Allocated memory at: %p\n\n", (void*)ptr);
    
    printf("Freeing memory (first time)...\n");
    free(ptr);
    printf("Memory freed\n\n");
    
    printf("Attempting to free same pointer again...\n");
    free(ptr);  // ASAN will catch this!
    
    printf("If you see this, ASAN is not enabled!\n");
    return 0;
}
