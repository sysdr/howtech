#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

// ANSI color codes
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// Measure time in nanoseconds
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Direct syscall using inline assembly (x86-64)
static inline long raw_syscall(long number, long arg1, long arg2, long arg3) {
    long ret;
    register long r10 asm("r10") = arg3;
    
    // Note: We mark rcx and r11 as clobbered because syscall destroys them
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(number), "D"(arg1), "S"(arg2), "r"(r10)
        : "rcx", "r11", "memory"
    );
    
    return ret;
}

// Test 1: Compare wrapper vs raw syscall
void test_wrapper_vs_raw(void) {
    printf("\n" COLOR_CYAN "═══════════════════════════════════════════════════════\n");
    printf("Test 1: Library Wrapper vs. Raw syscall Instruction\n");
    printf("═══════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    const int iterations = 1000000;
    uint64_t start, end;
    
    // Test with wrapper
    printf(COLOR_YELLOW "Testing getpid() wrapper (%d iterations)..." COLOR_RESET "\n", iterations);
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        getpid();
    }
    end = get_time_ns();
    double wrapper_ns = (double)(end - start) / iterations;
    printf(COLOR_GREEN "  Average: %.2f ns/call" COLOR_RESET "\n", wrapper_ns);
    
    // Test with raw syscall
    printf(COLOR_YELLOW "\nTesting raw syscall instruction (%d iterations)..." COLOR_RESET "\n", iterations);
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        raw_syscall(SYS_getpid, 0, 0, 0);
    }
    end = get_time_ns();
    double raw_ns = (double)(end - start) / iterations;
    printf(COLOR_GREEN "  Average: %.2f ns/call" COLOR_RESET "\n", raw_ns);
    
    printf("\n" COLOR_MAGENTA "Overhead: %.2f ns (%.1f%%)" COLOR_RESET "\n", 
           wrapper_ns - raw_ns, ((wrapper_ns - raw_ns) / raw_ns) * 100);
    
    printf(COLOR_CYAN "\n💡 Insight: " COLOR_RESET);
    printf("The wrapper adds ~2-5ns of overhead (function call, errno handling)\n");
}

// Test 2: Demonstrate register clobbering
void test_register_clobbering(void) {
    printf("\n" COLOR_CYAN "═══════════════════════════════════════════════════════\n");
    printf("Test 2: Register Clobbering (RCX and R11)\n");
    printf("═══════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    long rcx_before, r11_before, rcx_after, r11_after;
    
    // Save current values of rcx and r11 before syscall
    asm volatile("mov %%rcx, %0" : "=r"(rcx_before));
    asm volatile("mov %%r11, %0" : "=r"(r11_before));
    
    printf(COLOR_YELLOW "Before syscall:" COLOR_RESET "\n");
    printf("  RCX = " COLOR_GREEN "0x%016lx" COLOR_RESET "\n", rcx_before);
    printf("  R11 = " COLOR_GREEN "0x%016lx" COLOR_RESET "\n\n", r11_before);
    
    // Execute a syscall (getpid)
    printf(COLOR_YELLOW "Executing syscall (getpid)..." COLOR_RESET "\n\n");
    syscall(SYS_getpid);
    
    // Read values after syscall
    asm volatile("mov %%rcx, %0" : "=r"(rcx_after));
    asm volatile("mov %%r11, %0" : "=r"(r11_after));
    
    printf(COLOR_YELLOW "After syscall:" COLOR_RESET "\n");
    printf("  RCX = " COLOR_RED "0x%016lx" COLOR_RESET " ← Contains return address\n", rcx_after);
    printf("  R11 = " COLOR_RED "0x%016lx" COLOR_RESET " ← Contains old RFLAGS\n\n", r11_after);
    
    printf(COLOR_CYAN "💡 Insight: " COLOR_RESET);
    printf("RCX and R11 are destroyed by syscall/sysret.\n");
    printf("CPU uses them to save RIP and RFLAGS during the transition.\n");
    printf("Never rely on these registers across syscall boundaries!\n");
}

// Test 3: Measure syscall cost breakdown
void test_syscall_cost(void) {
    printf("\n" COLOR_CYAN "═══════════════════════════════════════════════════════\n");
    printf("Test 3: Syscall Cost Breakdown\n");
    printf("═══════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    const int warmup = 10000;
    const int iterations = 100000;
    uint64_t start, end;
    
    // Warmup
    for (int i = 0; i < warmup; i++) {
        syscall(SYS_getpid);
    }
    
    // Test null syscall (getpid - very fast)
    printf(COLOR_YELLOW "getpid() - Minimal work in kernel:" COLOR_RESET "\n");
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        syscall(SYS_getpid);
    }
    end = get_time_ns();
    printf("  " COLOR_GREEN "%.2f ns/call" COLOR_RESET "\n", (double)(end - start) / iterations);
    
    // Test syscall with more kernel work (gettid)
    printf(COLOR_YELLOW "\ngettid() - Similar to getpid:" COLOR_RESET "\n");
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        syscall(SYS_gettid);
    }
    end = get_time_ns();
    printf("  " COLOR_GREEN "%.2f ns/call" COLOR_RESET "\n", (double)(end - start) / iterations);
    
    // Test write syscall (involves more kernel work)
    printf(COLOR_YELLOW "\nwrite() to /dev/null - More kernel work:" COLOR_RESET "\n");
    int fd = open("/dev/null", 1); // O_WRONLY
    char buf[1] = {'x'};
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        write(fd, buf, 1);
    }
    end = get_time_ns();
    close(fd);
    printf("  " COLOR_GREEN "%.2f ns/call" COLOR_RESET "\n", (double)(end - start) / iterations);
    
    printf("\n" COLOR_CYAN "💡 Insight: " COLOR_RESET);
    printf("Syscall overhead is 70-150ns on modern CPUs.\n");
    printf("Kernel work adds additional time depending on the operation.\n");
    printf("With KPTI (Meltdown mitigation), costs increase by 20-30%%.\n");
}

// Test 4: Error handling demonstration
void test_error_handling(void) {
    printf("\n" COLOR_CYAN "═══════════════════════════════════════════════════════\n");
    printf("Test 4: Error Handling in Syscalls\n");
    printf("═══════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    printf(COLOR_YELLOW "Attempting to write to invalid file descriptor..." COLOR_RESET "\n");
    
    // Try to write to an invalid fd
    long ret = raw_syscall(SYS_write, 9999, (long)"test", 4);
    
    printf("Return value: " COLOR_RED "%ld" COLOR_RESET "\n", ret);
    
    if (ret < 0) {
        printf(COLOR_GREEN "✓ Syscall returned error: %ld (%s)" COLOR_RESET "\n", 
               -ret, strerror(-ret));
    }
    
    printf("\n" COLOR_CYAN "💡 Insight: " COLOR_RESET);
    printf("Syscalls return -errno on failure (negative error code).\n");
    printf("The C library wrapper converts this to -1 and sets errno.\n");
    printf("With raw syscalls, you get the kernel's actual return value.\n");
}

// Test 5: vDSO demonstration
void test_vdso(void) {
    printf("\n" COLOR_CYAN "═══════════════════════════════════════════════════════\n");
    printf("Test 5: vDSO vs Real Syscall\n");
    printf("═══════════════════════════════════════════════════════" COLOR_RESET "\n\n");
    
    const int iterations = 1000000;
    uint64_t start, end;
    struct timespec ts;
    
    printf(COLOR_YELLOW "Testing clock_gettime() (usually vDSO)..." COLOR_RESET "\n");
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &ts);
    }
    end = get_time_ns();
    double vdso_ns = (double)(end - start) / iterations;
    printf(COLOR_GREEN "  Average: %.2f ns/call" COLOR_RESET "\n", vdso_ns);
    
    printf(COLOR_YELLOW "\nTesting syscall(SYS_clock_gettime) (forces real syscall)..." COLOR_RESET "\n");
    start = get_time_ns();
    for (int i = 0; i < iterations; i++) {
        syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
    }
    end = get_time_ns();
    double syscall_ns = (double)(end - start) / iterations;
    printf(COLOR_GREEN "  Average: %.2f ns/call" COLOR_RESET "\n", syscall_ns);
    
    printf("\n" COLOR_MAGENTA "Speedup: %.1fx faster with vDSO" COLOR_RESET "\n", 
           syscall_ns / vdso_ns);
    
    printf(COLOR_CYAN "\n💡 Insight: " COLOR_RESET);
    printf("vDSO maps kernel code into user space for fast operations.\n");
    printf("No ring transition needed - just read from shared memory!\n");
    printf("Modern kernels use vDSO for: gettimeofday, clock_gettime, getcpu\n");
}

int main(int argc, char *argv[]) {
    printf(COLOR_CYAN "\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           Syscall Instruction Deep Dive                   ║\n");
    printf("║       Observing Kernel Entry and Exit Mechanisms          ║\n");
    printf("╔════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    
    printf("This demo shows what actually happens when your code\n");
    printf("crosses from user space (Ring 3) into kernel space (Ring 0).\n\n");
    
    printf(COLOR_YELLOW "Architecture: " COLOR_RESET);
    #if defined(__x86_64__)
        printf("x86-64 (syscall instruction)\n");
    #elif defined(__aarch64__)
        printf("ARM64 (svc instruction)\n");
    #else
        printf("Unknown\n");
    #endif
    
    // Run all tests
    test_wrapper_vs_raw();
    test_register_clobbering();
    test_syscall_cost();
    test_error_handling();
    test_vdso();
    
    printf("\n" COLOR_CYAN);
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                   Demo Complete!                          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET "\n");
    
    printf("To see syscalls in action, run:\n");
    printf("  " COLOR_GREEN "strace -c ./syscall_demo" COLOR_RESET "     # Count syscalls\n");
    printf("  " COLOR_GREEN "strace -T ./syscall_demo" COLOR_RESET "     # Show timing\n");
    printf("  " COLOR_GREEN "perf stat ./syscall_demo" COLOR_RESET "     # Performance counters\n\n");
    
    return 0;
}
