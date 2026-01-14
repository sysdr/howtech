#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define ANSI_CLEAR "\033[2J\033[H"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_RESET "\033[0m"

void print_header() {
    printf(ANSI_BOLD ANSI_BLUE);
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║              TSAN Race Condition Detection Monitor                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf(ANSI_RESET);
}

void print_status(const char* label, const char* value, const char* color) {
    printf("%s%-25s%s: %s%s%s\n", ANSI_BOLD, label, ANSI_RESET, color, value, ANSI_RESET);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program_name>\n", argv[0]);
        return 1;
    }
    
    print_header();
    printf("\n");
    
    print_status("Target Program", argv[1], ANSI_GREEN);
    print_status("Sanitizer", "ThreadSanitizer (TSAN)", ANSI_YELLOW);
    print_status("Detection Method", "Shadow Memory + Vector Clocks", ANSI_BLUE);
    
    printf("\n" ANSI_BOLD "What TSAN Detects:" ANSI_RESET "\n");
    printf("  • Data races (concurrent unsynchronized memory access)\n");
    printf("  • Read-write and write-write conflicts\n");
    printf("  • Missing synchronization primitives\n");
    printf("  • Incorrect memory ordering in atomics\n");
    
    printf("\n" ANSI_BOLD "Performance Overhead:" ANSI_RESET "\n");
    printf("  • Memory: " ANSI_YELLOW "5-10x" ANSI_RESET " (shadow memory)\n");
    printf("  • CPU: " ANSI_YELLOW "5-15x" ANSI_RESET " (instrumentation)\n");
    
    printf("\n" ANSI_BOLD ANSI_GREEN "Running instrumented program...\n" ANSI_RESET);
    printf("════════════════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}
