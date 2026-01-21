#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

void print_header() {
    printf(COLOR_BLUE);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           ASAN Memory Corruption Detector Monitor           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

void run_test(const char *name, const char *description, int delay) {
    printf(CLEAR_SCREEN);
    print_header();
    
    printf("\n%s[TEST]%s %s\n", COLOR_YELLOW, COLOR_RESET, name);
    printf("%s\n\n", description);
    
    printf("Status: %sRunning...%s\n", COLOR_GREEN, COLOR_RESET);
    printf("Press Ctrl+C to stop\n\n");
    
    sleep(delay);
}

int main() {
    printf(CLEAR_SCREEN);
    print_header();
    
    printf("\n%sStarting ASAN demonstration suite...%s\n\n", COLOR_GREEN, COLOR_RESET);
    sleep(2);
    
    const char *tests[] = {
        "Heap Buffer Overflow",
        "Use-After-Free",
        "Stack Buffer Overflow",
        "Memory Leak",
        "Double Free"
    };
    
    const char *descriptions[] = {
        "Writing past allocated buffer boundary",
        "Accessing memory after free()",
        "Overflowing stack-allocated buffer",
        "Allocating without freeing",
        "Freeing same pointer twice"
    };
    
    for (int i = 0; i < 5; i++) {
        run_test(tests[i], descriptions[i], 2);
    }
    
    printf(CLEAR_SCREEN);
    print_header();
    printf("\n%sAll tests completed!%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    return 0;
}
