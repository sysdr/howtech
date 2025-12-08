#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <ucontext.h>
#include <errno.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/time.h>

// Atomic flag for signal communication
static atomic_int signal_count = 0;
static atomic_int nested_depth = 0;

// Forward declarations
void print_ucontext(ucontext_t *uc);
void measure_signal_overhead(void);

// Safe signal handler - only async-signal-safe operations
void safe_signal_handler(int sig, siginfo_t *si, void *ucontext) {
    (void)sig;
    (void)si;
    (void)ucontext;
    int saved_errno = errno;
    
    // Safe operations only
    atomic_fetch_add(&signal_count, 1);
    
    // Write to stderr (async-signal-safe)
    const char msg[] = "\n[SIGNAL] Caught signal, examining context...\n";
    ssize_t written = write(STDERR_FILENO, msg, sizeof(msg) - 1);
    (void)written;  // Suppress unused result warning
    
    errno = saved_errno;
}

// Detailed signal handler that examines stack frame
void detailed_signal_handler(int sig, siginfo_t *si, void *ucontext) {
    int saved_errno = errno;
    ucontext_t *uc = (ucontext_t *)ucontext;
    
    atomic_fetch_add(&signal_count, 1);
    int depth = atomic_fetch_add(&nested_depth, 1);
    
    // Print signal information
    fprintf(stderr, "\n╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "  Signal Handler Invocation #%d (Nesting depth: %d)\n", 
            signal_count, depth);
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
    
    fprintf(stderr, "Signal Info:\n");
    fprintf(stderr, "  Number:  %d (%s)\n", sig, sig == SIGUSR1 ? "SIGUSR1" : "OTHER");
    fprintf(stderr, "  Code:    %d\n", si->si_code);
    fprintf(stderr, "  PID:     %d\n", si->si_pid);
    fprintf(stderr, "  UID:     %d\n", si->si_uid);
    
    // Examine stack pointer
    void *current_sp;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(current_sp));
    fprintf(stderr, "\nStack Information:\n");
    fprintf(stderr, "  Current RSP:     %p\n", current_sp);
    fprintf(stderr, "  ucontext addr:   %p\n", (void*)uc);
    fprintf(stderr, "  Stack size:      ~%ld bytes from ucontext\n", 
            (long)((char*)uc - (char*)current_sp));
    
    // Print some register values from saved context
    if (uc) {
        fprintf(stderr, "\nSaved CPU Context (from ucontext_t):\n");
        fprintf(stderr, "  RIP:    0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_RIP]);
        fprintf(stderr, "  RSP:    0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_RSP]);
        fprintf(stderr, "  RBP:    0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_RBP]);
        fprintf(stderr, "  RAX:    0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_RAX]);
        fprintf(stderr, "  RBX:    0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_RBX]);
        fprintf(stderr, "  EFLAGS: 0x%llx\n", (unsigned long long)uc->uc_mcontext.gregs[REG_EFL]);
        
        // Show signal mask
        fprintf(stderr, "\nSignal Mask (signals blocked during handler):\n");
        fprintf(stderr, "  Mask:   0x%016lx\n", uc->uc_sigmask.__val[0]);
    }
    
    fprintf(stderr, "════════════════════════════════════════════════════════════\n\n");
    
    atomic_fetch_sub(&nested_depth, 1);
    errno = saved_errno;
}

// Unsafe signal handler - demonstrates async-signal-safety violations
volatile int unsafe_counter = 0;
void unsafe_signal_handler(int sig) {
    (void)sig;
    // DANGER: malloc is NOT async-signal-safe!
    char *buf = malloc(100);
    if (buf) {
        sprintf(buf, "Unsafe handler called! Count: %d\n", ++unsafe_counter);
        printf("%s", buf);  // printf also not async-signal-safe!
        free(buf);
    }
}

// Function to register signal handlers
int setup_signal_handler(int signum, void (*handler)(int, siginfo_t*, void*), int use_altstack) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    
    if (use_altstack) {
        sa.sa_flags |= SA_ONSTACK;
    }
    
    // Block signal during handler execution (default behavior)
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, signum);
    
    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    
    return 0;
}

// Setup alternate signal stack
int setup_altstack(void) {
    stack_t ss;
    ss.ss_sp = malloc(SIGSTKSZ);
    if (ss.ss_sp == NULL) {
        perror("malloc");
        return -1;
    }
    
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    
    if (sigaltstack(&ss, NULL) == -1) {
        perror("sigaltstack");
        free(ss.ss_sp);
        return -1;
    }
    
    printf("✓ Alternate signal stack configured (%ld bytes)\n", (long)SIGSTKSZ);
    return 0;
}

// Measure signal delivery overhead
void measure_signal_overhead(void) {
    const int iterations = 10000;
    struct timespec start, end;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("  Performance Test: Signal Delivery Overhead\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    // Setup minimal handler
    setup_signal_handler(SIGUSR2, safe_signal_handler, 0);
    
    signal_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        kill(getpid(), SIGUSR2);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + 
                           (end.tv_nsec - start.tv_nsec);
    
    printf("\nResults:\n");
    printf("  Total signals:        %d\n", iterations);
    printf("  Signals handled:      %d\n", signal_count);
    printf("  Total time:           %lld ns\n", elapsed_ns);
    printf("  Time per signal:      %lld ns (~%.2f μs)\n", 
           elapsed_ns / iterations, 
           (double)elapsed_ns / iterations / 1000.0);
    printf("  Signals per second:   %.0f\n", 
           (double)iterations / ((double)elapsed_ns / 1000000000.0));
    printf("\n");
}

// Test nested signals (dangerous with SA_NODEFER)
void test_signal_queuing(void) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("  Test: Signal Queuing Behavior\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    signal_count = 0;
    
    printf("\nSending 5 SIGUSR1 signals rapidly...\n");
    printf("(Standard signals merge - you'll see < 5 handler invocations)\n\n");
    
    for (int i = 0; i < 5; i++) {
        kill(getpid(), SIGUSR1);
    }
    
    // Give time for signals to be delivered
    usleep(100000);
    
    printf("\nSignals sent: 5\n");
    printf("Handler invocations: %d\n", signal_count);
    printf("Signals merged: %d\n", 5 - signal_count);
    printf("\n");
}

// Demonstrate stack examination
void demonstrate_stack_frames(void) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("  Demonstration: Signal Frame Stack Layout\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    setup_signal_handler(SIGUSR1, detailed_signal_handler, 0);
    
    printf("\nSending SIGUSR1 to examine signal frame...\n");
    printf("Watch for ucontext_t structure and saved registers.\n");
    
    kill(getpid(), SIGUSR1);
    
    // Give handler time to complete
    usleep(10000);
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("\n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("  Signal Handling Internals: Linux Kernel Demo\n");
    printf("  Demonstrating signal delivery, stack frames, and context\n");
    printf("════════════════════════════════════════════════════════════════\n\n");
    
    // Setup alternate signal stack
    if (setup_altstack() != 0) {
        fprintf(stderr, "Warning: Could not setup alternate stack\n");
    }
    
    // Test 1: Examine signal frame and stack layout
    demonstrate_stack_frames();
    
    // Test 2: Measure signal delivery overhead
    measure_signal_overhead();
    
    // Test 3: Signal queuing behavior
    test_signal_queuing();
    
    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("  Demo Complete!\n");
    printf("════════════════════════════════════════════════════════════════\n\n");
    
    printf("Key Observations:\n");
    printf("  • Signal frame includes full ucontext_t (~2KB with AVX-512)\n");
    printf("  • Signal delivery overhead: 1-3 μs per signal\n");
    printf("  • Standard signals merge, real-time signals queue\n");
    printf("  • Alternate stack prevents stack overflow scenarios\n");
    printf("  • Only async-signal-safe functions allowed in handlers\n\n");
    
    printf("To trace syscalls, run:\n");
    printf("  strace -e signal=all ./build/signal_demo\n\n");
    
    printf("To examine with gdb:\n");
    printf("  gdb ./build/signal_demo\n");
    printf("  (gdb) handle SIGUSR1 nostop\n");
    printf("  (gdb) break detailed_signal_handler\n");
    printf("  (gdb) run\n");
    printf("  (gdb) bt    # Show stack trace in handler\n");
    printf("  (gdb) info frame\n");
    printf("  (gdb) x/100x $rsp    # Examine signal frame on stack\n\n");
    
    return 0;
}
