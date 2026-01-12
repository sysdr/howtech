#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

void print_header() {
    printf("\033[2J\033[H"); // Clear screen and move to top
    printf(BOLD CYAN "╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Symbol Versioning Demo - Real-Time Monitor              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n" RESET);
}

void print_section(const char *title) {
    printf("\n" BOLD YELLOW "▸ %s\n" RESET, title);
    printf("  ────────────────────────────────────────────────────────────────\n");
}

void run_command(const char *title, const char *cmd) {
    printf(BOLD GREEN "  [%s]" RESET "\n", title);
    printf(BLUE);
    (void)system(cmd);  // Intentionally ignore return value
    printf(RESET);
}

int main() {
    while (1) {
        print_header();
        
        print_section("Library Version Information");
        run_command("readelf", "readelf -V build/libmylib.so.3 2>/dev/null | grep -A 5 'Version definition\\|Version symbols' | head -20");
        
        print_section("Symbol Table with Versions");
        run_command("objdump", "objdump --dynamic-syms build/libmylib.so.3 2>/dev/null | grep 'MYLIB\\|api_' | head -15");
        
        print_section("Application Binary Requirements");
        run_command("v1.0 app", "readelf -V build/app_v1 2>/dev/null | grep -A 3 'Version needs' | head -10");
        
        print_section("System Libraries with Versioning");
        run_command("glibc", "readelf -V /lib/x86_64-linux-gnu/libc.so.6 2>/dev/null | grep 'GLIBC_' | head -8");
        
        printf("\n" CYAN "Refreshing in 5 seconds... (Ctrl+C to stop)" RESET "\n");
        sleep(5);
    }
    
    return 0;
}
