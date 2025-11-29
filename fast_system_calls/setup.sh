#!/bin/bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# Project directory
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

echo -e "${BOLD}${CYAN}======================================${NC}"
echo -e "${BOLD}${CYAN}   Fast Syscalls: getpid() vs vDSO   ${NC}"
echo -e "${BOLD}${CYAN}======================================${NC}\n"

# Create build directory
mkdir -p "${BUILD_DIR}"

# Create the benchmark program
cat > "${BUILD_DIR}/getpid_bench.c" << 'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>

#define ITERATIONS 10000000
#define NS_PER_SEC 1000000000L

static inline long long timespec_diff_ns(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * NS_PER_SEC + 
           (end->tv_nsec - start->tv_nsec);
}

// Direct syscall - bypasses glibc and vDSO completely
static inline pid_t raw_getpid(void) {
    return (pid_t)syscall(SYS_getpid);
}

int main(int argc, char **argv) {
    struct timespec start, end;
    long long elapsed_ns;
    double ns_per_call;
    int use_vdso = 1;
    
    if (argc > 1 && strcmp(argv[1], "--no-vdso") == 0) {
        use_vdso = 0;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         getpid() Performance Benchmark                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Configuration:\n");
    printf("  • Iterations: %d\n", ITERATIONS);
    printf("  • Method: %s\n", use_vdso ? "glibc getpid() with vDSO" : "raw syscall(SYS_getpid)");
    printf("\n");
    
    // Warmup
    for (int i = 0; i < 1000; i++) {
        if (use_vdso) {
            getpid();
        } else {
            raw_getpid();
        }
    }
    
    printf("Running benchmark");
    fflush(stdout);
    
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime start");
        return 1;
    }
    
    if (use_vdso) {
        // Use glibc's getpid() - will use vDSO if available
        for (int i = 0; i < ITERATIONS; i++) {
            getpid();
            if (i % (ITERATIONS / 10) == 0) {
                printf(".");
                fflush(stdout);
            }
        }
    } else {
        // Direct syscall - always goes to kernel
        for (int i = 0; i < ITERATIONS; i++) {
            raw_getpid();
            if (i % (ITERATIONS / 10) == 0) {
                printf(".");
                fflush(stdout);
            }
        }
    }
    
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime end");
        return 1;
    }
    
    elapsed_ns = timespec_diff_ns(&start, &end);
    ns_per_call = (double)elapsed_ns / ITERATIONS;
    
    printf(" Done!\n\n");
    
    printf("Results:\n");
    printf("  • Total time:    %.3f ms\n", elapsed_ns / 1000000.0);
    printf("  • Per call:      %.2f ns\n", ns_per_call);
    printf("  • Throughput:    %.2f M calls/sec\n", ITERATIONS / (elapsed_ns / 1000.0));
    printf("\n");
    
    if (use_vdso && ns_per_call < 20) {
        printf("✓ vDSO is working! Excellent performance (<20ns per call)\n");
    } else if (use_vdso && ns_per_call < 50) {
        printf("⚠ vDSO likely active but some overhead detected\n");
    } else if (!use_vdso) {
        printf("⚠ Direct syscall path (expected to be slower)\n");
    } else {
        printf("✗ vDSO may be disabled! Performance is unusually slow\n");
    }
    printf("\n");
    
    return 0;
}
EOF

# Create the vDSO inspector program
cat > "${BUILD_DIR}/vdso_inspector.c" << 'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/auxv.h>

void check_vdso_in_maps(void) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        perror("fopen /proc/self/maps");
        return;
    }
    
    char line[512];
    int found_vdso = 0;
    char vdso_address[32] = {0};
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         Checking for vDSO in Process Memory Map             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "[vdso]")) {
            found_vdso = 1;
            sscanf(line, "%31s", vdso_address);
            printf("✓ vDSO found in memory:\n");
            printf("  %s", line);
            break;
        }
    }
    
    if (!found_vdso) {
        printf("✗ vDSO NOT found in /proc/self/maps\n");
        printf("  This means fast syscalls are disabled!\n");
    }
    
    fclose(maps);
    printf("\n");
}

void print_auxv_info(void) {
    unsigned long vdso_addr = getauxval(AT_SYSINFO_EHDR);
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         Auxiliary Vector (AT_SYSINFO_EHDR)                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    if (vdso_addr != 0) {
        printf("✓ vDSO base address: 0x%lx\n", vdso_addr);
        printf("  The kernel has provided vDSO to this process\n");
    } else {
        printf("✗ AT_SYSINFO_EHDR not found\n");
        printf("  vDSO may not be available\n");
    }
    printf("\n");
}

void explain_vdso(void) {
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         What is vDSO?                                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("vDSO (Virtual Dynamic Shared Object) is a kernel mechanism that\n");
    printf("allows certain syscalls to execute entirely in user space.\n");
    printf("\n");
    printf("Benefits:\n");
    printf("  • No context switch overhead (ring 3 → ring 0)\n");
    printf("  • No register save/restore\n");
    printf("  • 20x faster than traditional syscalls\n");
    printf("\n");
    printf("Syscalls using vDSO:\n");
    printf("  • getpid()        - Process ID\n");
    printf("  • gettimeofday()  - Current time\n");
    printf("  • clock_gettime() - High-resolution time\n");
    printf("  • getcpu()        - Current CPU number\n");
    printf("\n");
}

int main(void) {
    printf("\n");
    explain_vdso();
    print_auxv_info();
    check_vdso_in_maps();
    
    printf("Next Steps:\n");
    printf("  1. Run 'cat /proc/self/maps | grep vdso' to see vDSO mapping\n");
    printf("  2. Use 'strace ./program' - vDSO calls won't appear!\n");
    printf("  3. Compare with raw syscall to see performance difference\n");
    printf("\n");
    
    return 0;
}
EOF

# Create the monitor program
cat > "${BUILD_DIR}/monitor.c" << 'EOF'
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <signal.h>

#define SAMPLE_DURATION_MS 100
#define TOTAL_SAMPLES 50

volatile sig_atomic_t stop_monitoring = 0;

void sigint_handler(int sig) {
    (void)sig;
    stop_monitoring = 1;
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void print_bar(const char *label, double value, double max, const char *color) {
    int bar_width = 40;
    int filled = (int)((value / max) * bar_width);
    
    printf("  %-20s [", label);
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("%s█%s", color, "\033[0m");
        } else {
            printf("░");
        }
    }
    printf("] %.1f%%\n", (value / max) * 100);
}

int main(void) {
    signal(SIGINT, sigint_handler);
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         Real-Time Syscall Monitor                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("This monitor demonstrates the difference between vDSO-accelerated\n");
    printf("and traditional syscalls in real-time.\n");
    printf("\n");
    printf("Press Ctrl+C to stop...\n\n");
    sleep(2);
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    unsigned long long vdso_calls = 0;
    unsigned long long syscalls = 0;
    
    for (int sample = 0; sample < TOTAL_SAMPLES && !stop_monitoring; sample++) {
        clear_screen();
        
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start.tv_sec) + 
                        (now.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║         Real-Time Syscall Performance Monitor               ║\n");
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("Runtime: %.1f seconds\n\n", elapsed);
        
        // Simulate vDSO calls (fast)
        struct timespec vdso_start, vdso_end;
        clock_gettime(CLOCK_MONOTONIC, &vdso_start);
        for (int i = 0; i < 100000; i++) {
            getpid();
            vdso_calls++;
        }
        clock_gettime(CLOCK_MONOTONIC, &vdso_end);
        double vdso_ms = (vdso_end.tv_sec - vdso_start.tv_sec) * 1000.0 +
                         (vdso_end.tv_nsec - vdso_start.tv_nsec) / 1e6;
        
        // Simulate real syscalls (slow)
        struct timespec syscall_start, syscall_end;
        clock_gettime(CLOCK_MONOTONIC, &syscall_start);
        for (int i = 0; i < 10000; i++) {
            syscall(SYS_getpid);
            syscalls++;
        }
        clock_gettime(CLOCK_MONOTONIC, &syscall_end);
        double syscall_ms = (syscall_end.tv_sec - syscall_start.tv_sec) * 1000.0 +
                           (syscall_end.tv_nsec - syscall_start.tv_nsec) / 1e6;
        
        printf("Call Statistics:\n");
        printf("  • vDSO calls:       %12llu  (%.2f M/sec)\n", 
               vdso_calls, vdso_calls / elapsed / 1e6);
        printf("  • Real syscalls:    %12llu  (%.2f M/sec)\n\n", 
               syscalls, syscalls / elapsed / 1e6);
        
        printf("Performance (100K iterations):\n");
        print_bar("vDSO getpid()", vdso_ms, syscall_ms, "\033[0;32m");
        print_bar("Raw syscall", syscall_ms, syscall_ms, "\033[0;31m");
        
        printf("\n");
        printf("Time per call:\n");
        printf("  • vDSO:      %6.1f ns/call  🚀\n", vdso_ms * 1e6 / 100000);
        printf("  • Syscall:   %6.1f ns/call  🐌\n", syscall_ms * 1e6 / 10000);
        printf("  • Speedup:   %6.1fx faster\n", syscall_ms * 10);
        
        printf("\n");
        printf("Key Insight: vDSO eliminates context switches entirely!\n");
        printf("Press Ctrl+C to stop monitoring...\n");
        
        usleep(SAMPLE_DURATION_MS * 1000);
    }
    
    printf("\n\nMonitoring stopped.\n\n");
    return 0;
}
EOF

# Create Makefile
cat > "${BUILD_DIR}/Makefile" << 'EOF'
CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -std=c11
TARGETS = getpid_bench vdso_inspector monitor

all: $(TARGETS)

getpid_bench: getpid_bench.c
	$(CC) $(CFLAGS) -o $@ $<

vdso_inspector: vdso_inspector.c
	$(CC) $(CFLAGS) -o $@ $<

monitor: monitor.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean
EOF

# Create Dockerfile
cat > "${BUILD_DIR}/Dockerfile" << 'EOF'
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    strace \
    linux-tools-generic \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app/

RUN make clean && make

CMD ["/bin/bash"]
EOF

echo -e "${YELLOW}Building programs...${NC}"
cd "${BUILD_DIR}"
make clean 2>/dev/null || true
make

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Build successful${NC}\n"

# Run vDSO inspector
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${CYAN}Step 1: Inspecting vDSO Availability${NC}"
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
./vdso_inspector

echo -e "\n${YELLOW}Press Enter to continue to benchmarks...${NC}"
read

# Run benchmark with vDSO
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${CYAN}Step 2: Benchmark with vDSO (Fast Path)${NC}"
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
./getpid_bench

echo -e "\n${YELLOW}Press Enter to run direct syscall benchmark...${NC}"
read

# Run benchmark without vDSO (direct syscall)
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${CYAN}Step 3: Benchmark with Direct Syscall (Slow Path)${NC}"
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
./getpid_bench --no-vdso

echo -e "\n${YELLOW}Press Enter to demonstrate strace behavior...${NC}"
read

# Demonstrate strace
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${CYAN}Step 4: strace Demonstration${NC}"
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo
echo -e "${YELLOW}Creating test program that calls getpid() 5 times...${NC}"

cat > test_getpid.c << 'EOF'
#include <stdio.h>
#include <unistd.h>

int main(void) {
    for (int i = 0; i < 5; i++) {
        pid_t pid = getpid();
        printf("PID: %d\n", pid);
    }
    return 0;
}
EOF

gcc -o test_getpid test_getpid.c

echo
echo -e "${GREEN}Running with strace - Notice NO getpid() syscalls appear!${NC}"
echo -e "${YELLOW}(vDSO intercepts them in user space)${NC}"
echo
strace -e trace=getpid ./test_getpid 2>&1 | head -20

echo
echo -e "\n${YELLOW}Press Enter to run the real-time monitor...${NC}"
read

# Run monitor
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${CYAN}Step 5: Real-Time Performance Monitor${NC}"
echo -e "${BOLD}${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
./monitor

echo
echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BOLD}${GREEN}Demo Complete!${NC}"
echo -e "${BOLD}${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo
echo -e "${CYAN}Key Takeaways:${NC}"
echo -e "  ${GREEN}✓${NC} vDSO eliminates syscall overhead completely"
echo -e "  ${GREEN}✓${NC} getpid() with vDSO: ~5ns vs ~100-200ns without"
echo -e "  ${GREEN}✓${NC} strace can't see vDSO calls (they're in user space)"
echo -e "  ${GREEN}✓${NC} glibc caches PID for even better performance"
echo
echo -e "${YELLOW}Run ./cleanup.sh to remove all generated files${NC}"
echo