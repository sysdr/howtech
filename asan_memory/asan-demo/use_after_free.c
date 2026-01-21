#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== Use-After-Free Test ===\n");
    
    char *buffer = malloc(100);
    strcpy(buffer, "Hello from ASAN demo!");
    
    printf("Allocated buffer at: %p\n", (void*)buffer);
    printf("Buffer contents: %s\n\n", buffer);
    
    printf("Freeing buffer...\n");
    free(buffer);
    printf("Buffer freed (now in quarantine)\n\n");
    
    printf("Attempting to read freed memory...\n");
    printf("Freed buffer contains: %s\n", buffer);  // ASAN will catch this!
    
    printf("If you see this, ASAN is not enabled!\n");
    return 0;
}
