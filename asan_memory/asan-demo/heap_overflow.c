#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== Heap Buffer Overflow Test ===\n");
    
    int *array = malloc(10 * sizeof(int));
    printf("Allocated array at: %p\n", (void*)array);
    printf("Array size: 10 integers (40 bytes)\n\n");
    
    printf("Writing to valid indices (0-9)...\n");
    for (int i = 0; i < 10; i++) {
        array[i] = i * 10;
        printf("  array[%d] = %d\n", i, array[i]);
    }
    
    printf("\n%s\n", "Now attempting out-of-bounds write...");
    printf("Writing to array[12] (offset 48 - past the end)...\n");
    array[12] = 999;  // ASAN will catch this!
    
    printf("If you see this, ASAN is not enabled!\n");
    
    free(array);
    return 0;
}
