#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>

static sigjmp_buf segv_jmp;
static volatile sig_atomic_t caught_segv = 0;

void segv_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    caught_segv = 1;
    siglongjmp(segv_jmp, 1);
}

void print_memory_map() {
    printf("\n═══ Current Process Memory Map ═══\n");
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        perror("fopen /proc/self/maps");
        return;
    }
    
    char line[512];
    int user_regions = 0, kernel_visible = 0;
    
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[5], dev[6], path[256] = {0};
        unsigned long offset, inode;
        
        sscanf(line, "%lx-%lx %4s %lx %5s %lu %255[^\n]", 
               &start, &end, perms, &offset, dev, &inode, path);
        
        // Check if canonical low half (user space)
        if (start < 0x0000800000000000UL) {
            user_regions++;
            printf("  0x%016lx-0x%016lx %s", start, end, perms);
            if (strlen(path) > 0 && path[0] != ' ') {
                printf(" %s", path);
            }
            printf("\n");
        } else if (start >= 0xFFFF800000000000UL) {
            kernel_visible++;
            printf("  [KERNEL] 0x%016lx-0x%016lx %s", start, end, perms);
            if (strlen(path) > 0) {
                printf(" %s", path);
            }
            printf("\n");
        }
    }
    
    fclose(maps);
    printf("\nTotal user regions: %d\n", user_regions);
    if (kernel_visible > 0) {
        printf("Kernel-mapped regions visible: %d (vDSO/vsyscall)\n", kernel_visible);
    }
}

void test_non_canonical_address() {
    printf("\n═══ Testing Non-Canonical Address Access ═══\n");
    
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        return;
    }
    
    // Non-canonical address (in the hole)
    void *non_canonical = (void *)0x0000800000000000UL;
    
    printf("Attempting to access non-canonical address: %p\n", non_canonical);
    
    if (sigsetjmp(segv_jmp, 1) == 0) {
        caught_segv = 0;
        volatile char c = *(char *)non_canonical;
        (void)c;
        printf("ERROR: Should have segfaulted!\n");
    } else {
        printf("✓ Caught SIGSEGV as expected\n");
        printf("  Non-canonical addresses cause immediate segfault\n");
        printf("  The CPU rejects addresses where bits 47-63 don't match bit 47\n");
    }
    
    // Restore default handler
    signal(SIGSEGV, SIG_DFL);
}

void measure_syscall_overhead() {
    printf("\n═══ Syscall vs vDSO Performance ═══\n");
    
    const int iterations = 1000000;
    struct timespec start, end;
    
    // Measure getpid() - real syscall
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        getpid();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long getpid_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
                     (end.tv_nsec - start.tv_nsec);
    
    // Measure clock_gettime() - vDSO
    clock_gettime(CLOCK_MONOTONIC, &start);
    struct timespec dummy;
    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &dummy);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long vdso_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
                   (end.tv_nsec - start.tv_nsec);
    
    printf("Performance over %d iterations:\n", iterations);
    printf("  getpid() (real syscall):     %ld ns total (%.1f ns/call)\n", 
           getpid_ns, (double)getpid_ns / iterations);
    printf("  clock_gettime() (vDSO):      %ld ns total (%.1f ns/call)\n", 
           vdso_ns, (double)vdso_ns / iterations);
    printf("  Speedup: %.1fx\n", (double)getpid_ns / vdso_ns);
    printf("\nvDSO calls don't appear in strace because they're not syscalls!\n");
}

void show_page_fault_stats() {
    printf("\n═══ Page Fault Statistics ═══\n");
    
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        perror("getrusage");
        return;
    }
    
    printf("Minor page faults: %ld (TLB miss, page in memory)\n", usage.ru_minflt);
    printf("Major page faults: %ld (TLB miss, page on disk)\n", usage.ru_majflt);
    printf("Voluntary context switches: %ld\n", usage.ru_nvcsw);
    printf("Involuntary context switches: %ld\n", usage.ru_nivcsw);
}

void demonstrate_memory_regions() {
    printf("\n═══ Demonstrating Memory Regions ═══\n");
    
    // Stack variable
    int stack_var = 42;
    printf("Stack variable:  %p (should be near 0x00007FFF_FFFF_FFFF)\n", (void*)&stack_var);
    
    // Heap allocation
    void *heap_ptr = malloc(1024);
    printf("Heap allocation: %p (grows upward from brk)\n", heap_ptr);
    
    // mmap allocation
    void *mmap_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_ptr != MAP_FAILED) {
        printf("mmap allocation: %p (in mmap region)\n", mmap_ptr);
        munmap(mmap_ptr, 4096);
    }
    
    // Code segment
    printf("Code segment:    %p (executable .text)\n", (void*)&demonstrate_memory_regions);
    
    // Global data
    static int global_data = 123;
    printf("Global data:     %p (.data segment)\n", (void*)&global_data);
    
    free(heap_ptr);
}

void show_aslr_status() {
    printf("\n═══ ASLR Status ═══\n");
    FILE *aslr = fopen("/proc/sys/kernel/randomize_va_space", "r");
    if (aslr) {
        int value;
        if (fscanf(aslr, "%d", &value) == 1) {
            printf("ASLR setting: %d ", value);
            switch(value) {
                case 0: printf("(disabled)\n"); break;
                case 1: printf("(mmap base, stack, vDSO randomized)\n"); break;
                case 2: printf("(full randomization including heap)\n"); break;
                default: printf("(unknown)\n"); break;
            }
        }
        fclose(aslr);
    }
    
    printf("\nRun this program multiple times and watch addresses change!\n");
}

int main() {
    printf("Process PID: %d\n", getpid());
    printf("Canonical address ranges:\n");
    printf("  User:   0x0000_0000_0000_0000 to 0x0000_7FFF_FFFF_FFFF\n");
    printf("  Kernel: 0xFFFF_8000_0000_0000 to 0xFFFF_FFFF_FFFF_FFFF\n");
    
    print_memory_map();
    demonstrate_memory_regions();
    show_aslr_status();
    test_non_canonical_address();
    measure_syscall_overhead();
    show_page_fault_stats();
    
    printf("\n═══ Explore Further ═══\n");
    printf("Try these commands:\n");
    printf("  cat /proc/%d/maps     # See full memory map\n", getpid());
    printf("  cat /proc/%d/smaps    # Detailed memory statistics\n", getpid());
    printf("  pmap %d               # Formatted memory map\n", getpid());
    printf("  strace ./memexplore   # Trace syscalls\n");
    
    return 0;
}
