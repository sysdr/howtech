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
