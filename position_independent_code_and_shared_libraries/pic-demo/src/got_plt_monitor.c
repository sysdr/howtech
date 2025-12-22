#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define CLEAR "\033[2J\033[H"
#define BOLD "\033[1m"
#define GREEN "\033[0;32m"
#define BLUE "\033[0;34m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[0;36m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

extern void library_function(void);
extern int get_global(void);

void print_header(void) {
    printf(CLEAR);
    printf(BOLD BLUE "╔═══════════════════════════════════════════════════════════════════╗\n" RESET);
    printf(BOLD BLUE "║       GOT/PLT CALL OVERHEAD MONITOR - REAL-TIME METRICS          ║\n" RESET);
    printf(BOLD BLUE "╚═══════════════════════════════════════════════════════════════════╝\n" RESET);
    printf("\n");
}

void benchmark_calls(void) {
    const int iterations = 1000000;
    uint64_t start, end;
    
    // Warm up
    for (int i = 0; i < 1000; i++) {
        library_function();
    }
    
    print_header();
    
    // Measure PLT call overhead
    start = rdtsc();
    for (int i = 0; i < iterations; i++) {
        library_function();
    }
    end = rdtsc();
    
    uint64_t plt_cycles = (end - start) / iterations;
    
    printf(BOLD "  FUNCTION CALL OVERHEAD ANALYSIS\n" RESET);
    printf("  ════════════════════════════════════════════════════════════\n\n");
    
    printf("  " GREEN "●" RESET " Iterations:        %s%d%s calls\n", YELLOW, iterations, RESET);
    printf("  " GREEN "●" RESET " PLT Call Overhead: %s~%lu%s CPU cycles per call\n", CYAN, plt_cycles, RESET);
    printf("  " GREEN "●" RESET " Estimated Time:    %s~%.2f ns%s per call (at 3GHz)\n\n", 
           CYAN, plt_cycles / 3.0, RESET);
    
    printf(BOLD "  PLT MECHANISM STAGES:\n" RESET);
    printf("  ════════════════════════════════════════════════════════════\n\n");
    printf("  " YELLOW "1." RESET " call library_function@plt   " GREEN "→" RESET " Jump to PLT stub\n");
    printf("  " YELLOW "2." RESET " jmp *GOT[N]                 " GREEN "→" RESET " Indirect through GOT\n");
    printf("  " YELLOW "3." RESET " Execute function            " GREEN "→" RESET " Target code runs\n\n");
    
    printf(BOLD "  DYNAMIC LINKING STATUS:\n" RESET);
    printf("  ════════════════════════════════════════════════════════════\n\n");
    
    FILE *maps = fopen("/proc/self/maps", "r");
    if (maps) {
        char line[512];
        int shared_count = 0;
        printf("  " BOLD "Shared Libraries Loaded:" RESET "\n\n");
        while (fgets(line, sizeof(line), maps)) {
            if (strstr(line, "r-xp") && strstr(line, ".so")) {
                printf("  " GREEN "●" RESET " %s", line);
                shared_count++;
            }
        }
        fclose(maps);
        printf("\n  Total shared libraries: %s%d%s\n\n", CYAN, shared_count, RESET);
    }
    
    printf(BOLD "  MEMORY EFFICIENCY:\n" RESET);
    printf("  ════════════════════════════════════════════════════════════\n\n");
    printf("  " GREEN "✓" RESET " Text segments shared across all processes\n");
    printf("  " GREEN "✓" RESET " GOT/PLT indirection enables ASLR\n");
    printf("  " GREEN "✓" RESET " One indirect jump vs symbol resolution\n\n");
    
    printf(BOLD "  TRADE-OFFS:\n" RESET);
    printf("  ════════════════════════════════════════════════════════════\n\n");
    printf("  " YELLOW "Cost:" RESET "    ~%lu cycles per external call\n", plt_cycles);
    printf("  " GREEN "Benefit:" RESET " Memory sharing + ASLR security\n\n");
}

int main(void) {
    benchmark_calls();
    return 0;
}
