#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <errno.h>

#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

void print_header() {
    printf(COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           PLT/GOT Dynamic Linking Monitor                    ║\n");
    printf("║  Tracking lazy symbol resolution in real-time               ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program> [args...]\n", argv[0]);
        return 1;
    }
    
    print_header();
    
    printf(COLOR_YELLOW "\n[Monitor] Starting target program: %s\n" COLOR_RESET, argv[1]);
    printf(COLOR_BLUE "[Info] Set LD_DEBUG=all for verbose dynamic linker output\n" COLOR_RESET);
    printf(COLOR_BLUE "[Info] Use 'readelf -r' to see relocation entries\n" COLOR_RESET);
    printf(COLOR_BLUE "[Info] Use 'objdump -d -j .plt' to see PLT stubs\n\n" COLOR_RESET);
    
    // Execute with LD_DEBUG to show dynamic linker activity
    setenv("LD_DEBUG", "bindings", 1);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child: execute target program
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent: wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf(COLOR_GREEN "\n[Monitor] Target program exited with code %d\n" COLOR_RESET, 
                   WEXITSTATUS(status));
        }
    } else {
        perror("fork");
        return 1;
    }
    
    return 0;
}
