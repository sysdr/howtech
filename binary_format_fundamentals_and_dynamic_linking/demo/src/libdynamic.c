#include <stdio.h>
#include <unistd.h>

int expensive_calculation(int n) {
    // Simulate some work
    int result = 0;
    for (int i = 0; i < n * 1000; i++) {
        result += i;
    }
    printf("expensive_calculation(%d) = %d\n", n, result);
    return result;
}

void print_message(const char* msg) {
    printf("Message: %s\n", msg);
}
