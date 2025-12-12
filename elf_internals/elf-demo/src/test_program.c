#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    printf("Hello from ELF binary!\n");
    printf("Second call to printf\n");
    printf("Third call to printf\n");
    
    for (int i = 0; i < 100; i++) {
        printf("Call #%d\n", i);
    }
    
    return 0;
}
