#include <stdio.h>
#include <stdlib.h>

void leaky_function(int size) {
    int *leak = malloc(size * sizeof(int));
    leak[0] = 42;
    printf("Allocated %d bytes at %p (never freed)\n", size * 4, (void*)leak);
    // Intentionally NOT calling free(leak)
}

int main() {
    printf("=== Memory Leak Detection Test ===\n");
    printf("Allocating memory without freeing...\n\n");
    
    for (int i = 0; i < 5; i++) {
        leaky_function(100 + i * 50);
    }
    
    printf("\nProgram ending - ASAN will report leaks...\n");
    return 0;
}
