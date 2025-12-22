#include <stdio.h>

int global_data = 42;

void library_function(void) {
    printf("Library function called! global_data=%d\n", global_data);
    global_data++;
}

int get_global(void) {
    return global_data;
}

void modify_global(int val) {
    global_data = val;
}
