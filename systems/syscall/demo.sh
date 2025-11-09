#!/bin/bash

# Syscall Deep Dive Demo
# This script creates all necessary files, builds, and demonstrates syscall behavior

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

echo -e "${CYAN}"
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Dissecting the syscall Instruction Demo                 ║"
echo "║   Kernel Entry and Exit Mechanisms                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo -e "${NC}\n"

# Check if running as root for perf (optional)
if [ "$EUID" -ne 0 ]; then 
    echo -e "${YELLOW}Note: Not running as root. Some perf features may be limited.${NC}"
    echo -e "${YELLOW}Run with sudo for full perf capabilities.${NC}\n"
fi

#############################################
# Step 1: Create source files
#############################################

echo -e "${BLUE}[1/6] Creating source files...${NC}"

# Create the main demo C file
cat > syscall_demo.c << 'EOF'
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
EOF

echo -e "${GREEN}✓${NC} Created syscall_demo.c"

# Create a simple terminal monitor
cat > monitor.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define COLOR_CYAN    "\x1b[36m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RESET   "\x1b[0m"

int main(void) {
    printf(COLOR_CYAN "\n╔═══════════════════════════════════════════════════╗\n");
    printf("║         Real-time Syscall Monitor                ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n" COLOR_RESET);
    
    printf("\nRun in another terminal:\n");
    printf(COLOR_YELLOW "  strace -c -p $(pgrep syscall_demo)\n" COLOR_RESET);
    printf(COLOR_YELLOW "  perf trace -p $(pgrep syscall_demo)\n" COLOR_RESET);
    
    printf("\nMonitoring syscalls via /proc...\n\n");
    
    printf(COLOR_GREEN "PID\t\tSyscall\t\tArgs\n" COLOR_RESET);
    printf("────────────────────────────────────────────────\n");
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep syscall_demo");
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char pid[16];
        while (fgets(pid, sizeof(pid), fp)) {
            pid[strcspn(pid, "\n")] = 0;
            char syscall_path[256];
            snprintf(syscall_path, sizeof(syscall_path), "/proc/%s/syscall", pid);
            
            FILE *sf = fopen(syscall_path, "r");
            if (sf) {
                char line[256];
                if (fgets(line, sizeof(line), sf)) {
                    printf("%s\t%s", pid, line);
                }
                fclose(sf);
            }
        }
        pclose(fp);
    }
    
    printf("\n" COLOR_CYAN "Note: " COLOR_RESET "This is a simple monitor.\n");
    printf("For real-time tracing, use: " COLOR_GREEN "perf trace" COLOR_RESET " or " COLOR_GREEN "strace" COLOR_RESET "\n\n");
    
    return 0;
}
EOF

echo -e "${GREEN}✓${NC} Created monitor.c"

#############################################
# Step 2: Create Makefile
#############################################

echo -e "\n${BLUE}[2/6] Creating Makefile...${NC}"

cat > Makefile << 'EOF'
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11
DEBUGFLAGS = -g -O0

all: syscall_demo monitor

syscall_demo: syscall_demo.c
	$(CC) $(CFLAGS) -o syscall_demo syscall_demo.c

monitor: monitor.c
	$(CC) $(CFLAGS) -o monitor monitor.c

debug: syscall_demo.c
	$(CC) $(DEBUGFLAGS) -o syscall_demo_debug syscall_demo.c

clean:
	rm -f syscall_demo monitor syscall_demo_debug *.o

.PHONY: all clean debug
EOF

echo -e "${GREEN}✓${NC} Created Makefile"

#############################################
# Step 3: Create Dockerfile
#############################################

echo -e "\n${BLUE}[3/6] Creating Dockerfile...${NC}"

cat > Dockerfile << 'EOF'
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    strace \
    linux-tools-generic \
    gdb \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /demo

COPY syscall_demo.c monitor.c Makefile ./

RUN make all

CMD ["./syscall_demo"]
EOF

echo -e "${GREEN}✓${NC} Created Dockerfile"

#############################################
# Step 4: Install dependencies
#############################################

echo -e "\n${BLUE}[4/6] Checking and installing dependencies...${NC}"

# Detect OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS - skip apt-get
    if ! command -v gcc &> /dev/null; then
        echo -e "${YELLOW}gcc not found. Install Xcode Command Line Tools:${NC}"
        echo -e "${YELLOW}  xcode-select --install${NC}"
        exit 1
    fi
    if ! command -v strace &> /dev/null; then
        echo -e "${YELLOW}Note: strace not available on macOS. Install with: brew install strace${NC}"
        echo -e "${YELLOW}Or use dtruss/dtrace for tracing.${NC}"
    fi
else
    # Linux - use apt-get
    if ! command -v gcc &> /dev/null; then
        echo -e "${YELLOW}Installing build-essential...${NC}"
        apt-get update -qq && apt-get install -y -qq build-essential
    fi
    
    # Check for strace
    if ! command -v strace &> /dev/null; then
        echo -e "${YELLOW}Installing strace...${NC}"
        apt-get install -y -qq strace
    fi
fi

echo -e "${GREEN}✓${NC} Dependencies ready"

#############################################
# Step 5: Compile the code
#############################################

echo -e "\n${BLUE}[5/6] Compiling demo programs...${NC}"

# Check if we're on macOS and Docker is available
if [[ "$OSTYPE" == "darwin"* ]] && command -v docker &> /dev/null; then
    echo -e "${YELLOW}Detected macOS. This demo requires Linux syscalls.${NC}"
    
    # Check if Docker daemon is running
    if ! docker info &> /dev/null; then
        echo -e "${RED}Docker daemon is not running.${NC}"
        echo -e "${YELLOW}Please start Docker Desktop and try again.${NC}"
        echo -e "${YELLOW}Or run natively on Linux.${NC}\n"
        exit 1
    fi
    
    echo -e "${YELLOW}Building Docker image and running in container...${NC}\n"
    
    # Build Docker image with x86-64 platform
    docker build --platform linux/amd64 -t syscall-demo . > /tmp/docker_build.log 2>&1
    BUILD_RESULT=$?
    
    if [ $BUILD_RESULT -eq 0 ]; then
        echo -e "\n${GREEN}✓${NC} Docker image built successfully"
        echo -e "\n${BLUE}[6/6] Running syscall demonstrations in Docker...${NC}\n"
        docker run --platform linux/amd64 --rm syscall-demo
        exit 0
    else
        echo -e "${RED}✗${NC} Docker build failed"
        echo -e "${YELLOW}Build log:${NC}"
        tail -20 /tmp/docker_build.log
        exit 1
    fi
fi

# Native Linux compilation
make clean &> /dev/null || true
make all

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Compilation successful"
    echo -e "  ${GREEN}→${NC} syscall_demo"
    echo -e "  ${GREEN}→${NC} monitor"
else
    echo -e "${RED}✗${NC} Compilation failed"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo -e "${YELLOW}This demo requires Linux. Try running in Docker:${NC}"
        echo -e "  ${GREEN}docker build -t syscall-demo .${NC}"
        echo -e "  ${GREEN}docker run --rm syscall-demo${NC}"
    fi
    exit 1
fi

#############################################
# Step 6: Run demonstrations
#############################################

echo -e "\n${BLUE}[6/6] Running syscall demonstrations...${NC}\n"

# Run the main demo
./syscall_demo

# Show strace summary
if command -v strace &> /dev/null; then
    echo -e "\n${CYAN}════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}Bonus: strace syscall count${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════${NC}\n"
    
    strace -c ./syscall_demo 2>&1 | tail -20
else
    echo -e "\n${YELLOW}Note: strace not available. Skipping syscall count.${NC}"
    echo -e "${YELLOW}On macOS, you can use: dtruss -c ./syscall_demo${NC}\n"
fi

# Optional: Docker demonstration
if command -v docker &> /dev/null; then
    echo -e "\n${CYAN}════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}Optional: Run in Docker container${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════${NC}\n"
    echo -e "To build and run in Docker:"
    echo -e "  ${GREEN}docker build -t syscall-demo .${NC}"
    echo -e "  ${GREEN}docker run --rm syscall-demo${NC}\n"
fi

echo -e "${GREEN}════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}Demo complete! Key takeaways:${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════${NC}\n"

echo -e "1. ${YELLOW}syscall${NC} instruction costs ~70-150ns with modern mitigations"
echo -e "2. ${YELLOW}RCX and R11${NC} are always clobbered - never rely on them"
echo -e "3. ${YELLOW}vDSO${NC} avoids syscalls entirely for time-related calls"
echo -e "4. ${YELLOW}KPTI${NC} (Meltdown mitigation) adds 20-30% overhead"
echo -e "5. ${YELLOW}strace/perf${NC} are essential tools for syscall analysis\n"

echo -e "${CYAN}To explore further:${NC}"
echo -e "  • Examine assembly: ${GREEN}objdump -d syscall_demo | less${NC}"
echo -e "  • Debug with gdb: ${GREEN}gdb ./syscall_demo${NC}"
echo -e "  • Trace with perf: ${GREEN}perf trace ./syscall_demo${NC}"
echo -e "  • Check vDSO: ${GREEN}ldd syscall_demo${NC}\n"

echo -e "${GREEN}✨ Happy syscall hacking! ✨${NC}\n"