#include <stdio.h>

void runtime_loaded_function(void) {
    printf("This function was loaded at runtime with dlopen!\n");
    printf("It wasn't linked at compile time.\n");
}
