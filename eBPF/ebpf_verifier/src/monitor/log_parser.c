#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

void print_header(void) {
    printf("\n%s%s", BOLD, CYAN);
    printf("╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║              eBPF Verifier Log Analysis Monitor                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════╝\n");
    printf("%s\n", RESET);
}

void print_section(const char *title) {
    printf("\n%s%s▶ %s%s\n", BOLD, BLUE, title, RESET);
    printf("%s", BLUE);
    for (int i = 0; i < 76; i++) printf("─");
    printf("%s\n", RESET);
}

void analyze_log(const char *filename, const char *program_name, int should_pass) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("%s✗ Could not open log file: %s%s\n", RED, filename, RESET);
        return;
    }
    
    char line[1024];
    int insn_count = 0;
    int state_explored = 0;
    int error_found = 0;
    char error_msg[512] = "";
    
    printf("\n%s%s╭─ Program: %s %s\n", BOLD, YELLOW, program_name, RESET);
    printf("%s%s│  Expected: %s%s\n", BOLD, YELLOW, 
           should_pass ? "PASS" : "FAIL", RESET);
    printf("%s%s╰─────────────────────────────────────────────────────────────────%s\n", 
           BOLD, YELLOW, RESET);
    
    while (fgets(line, sizeof(line), fp)) {
        // Count instructions processed
        if (strstr(line, "processed") && strstr(line, "insns")) {
            sscanf(line, "%d:", &insn_count);
        }
        
        // Count states explored
        if (strstr(line, "from ")) {
            state_explored++;
        }
        
        // Look for errors
        if (strstr(line, "invalid") || strstr(line, "not allowed") || 
            strstr(line, "R") && (strstr(line, "unbounded") || strstr(line, "!read_ok"))) {
            error_found = 1;
            strncpy(error_msg, line, sizeof(error_msg) - 1);
            error_msg[sizeof(error_msg) - 1] = '\0';
            // Trim newline
            char *newline = strchr(error_msg, '\n');
            if (newline) *newline = '\0';
        }
    }
    
    fclose(fp);
    
    // Print analysis
    printf("\n  %s%sInstructions Processed:%s %d\n", BOLD, CYAN, RESET, insn_count);
    printf("  %s%sStates Explored:%s %d\n", BOLD, CYAN, RESET, state_explored);
    
    if (error_found) {
        printf("\n  %s%s✗ VERIFICATION FAILED%s\n", BOLD, RED, RESET);
        printf("  %sError:%s %s\n", RED, RESET, error_msg);
    } else {
        printf("\n  %s%s✓ VERIFICATION PASSED%s\n", BOLD, GREEN, RESET);
        printf("  %sProgram is memory-safe and ready for JIT compilation%s\n", 
               GREEN, RESET);
    }
    
    printf("\n");
}

void show_verification_stages(void) {
    print_section("Verification Pipeline");
    
    printf("%s  1. DAG Construction%s\n", CYAN, RESET);
    printf("     └─ Build control flow graph\n");
    printf("     └─ Detect unreachable code\n\n");
    
    printf("%s  2. Register State Tracking%s\n", CYAN, RESET);
    printf("     └─ R0-R10 type and range tracking\n");
    printf("     └─ Symbolic execution per instruction\n\n");
    
    printf("%s  3. Safety Verification%s\n", CYAN, RESET);
    printf("     └─ Bounds checking on all memory access\n");
    printf("     └─ Helper function argument validation\n\n");
    
    printf("%s  4. Complexity Analysis%s\n", CYAN, RESET);
    printf("     └─ Instruction count < 1M\n");
    printf("     └─ Complexity units < 4096 per path\n");
    printf("     └─ Stack usage < 512 bytes\n\n");
}

void show_register_types(void) {
    print_section("Common Register State Types");
    
    printf("  %-25s %s\n", "scalar(min-max)", "Integer with value range");
    printf("  %-25s %s\n", "pkt(off=N,r=M)", "Packet pointer, range M bytes");
    printf("  %-25s %s\n", "pkt_end(off=N)", "Packet end boundary");
    printf("  %-25s %s\n", "ctx", "Context pointer (xdp_md, etc)");
    printf("  %-25s %s\n", "map_value", "Map value pointer");
    printf("  %-25s %s\n", "fp (stack_ptr)", "Frame pointer (R10)");
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode> [log_files...]\n", argv[0]);
        printf("Modes: stages, registers, analyze\n");
        return 1;
    }
    
    print_header();
    
    if (strcmp(argv[1], "stages") == 0) {
        show_verification_stages();
    } else if (strcmp(argv[1], "registers") == 0) {
        show_register_types();
    } else if (strcmp(argv[1], "analyze") == 0 && argc >= 3) {
        show_verification_stages();
        for (int i = 2; i < argc; i += 2) {
            if (i + 1 < argc) {
                int should_pass = strcmp(argv[i+1], "pass") == 0;
                analyze_log(argv[i], argv[i], should_pass);
            }
        }
    }
    
    return 0;
}
