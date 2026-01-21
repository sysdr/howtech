#include <stdio.h>
#include <string.h>

void vulnerable_function() {
    char buffer[8];
    
    printf("=== Stack Buffer Overflow Test ===\n");
    printf("Stack buffer size: 8 bytes\n");
    printf("Buffer address: %p\n\n", (void*)buffer);
    
    printf("Attempting to copy 30-byte string into 8-byte buffer...\n");
    strcpy(buffer, "This string is way too long!");  // ASAN will catch this!
    
    printf("Buffer contents: %s\n", buffer);
    printf("If you see this, ASAN is not enabled!\n");
}

int main() {
    vulnerable_function();
    return 0;
}
