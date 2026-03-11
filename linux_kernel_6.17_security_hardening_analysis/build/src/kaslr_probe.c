/*
 * kaslr_probe.c — Observe KASLR from user space (information-leak mitigations)
 *
 * Demonstrates kptr_restrict and dmesg_restrict in action.
 * Shows what attackers used to leak from /proc/kallsyms before 6.x hardening,
 * and what they see now with kptr_restrict=2.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>

#define COL_GREEN  "\033[0;32m"
#define COL_RED    "\033[0;31m"
#define COL_YELLOW "\033[1;33m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[0;36m"
#define COL_RESET  "\033[0m"

static void probe_kallsyms(void)
{
    printf("\n  " COL_BOLD "kallsyms leak probe (kptr_restrict effect):" COL_RESET "\n");

    FILE *f = fopen("/proc/kallsyms", "r");
    if (!f) {
        printf("  Cannot open /proc/kallsyms: %s\n", strerror(errno));
        return;
    }

    char line[256];
    int lines_read = 0;
    int zero_addrs = 0;
    int real_addrs = 0;
    const char *sample_sym  = NULL;
    unsigned long sample_addr = 0;

    while (fgets(line, sizeof(line), f) && lines_read < 10000) {
        unsigned long addr = 0;
        char type[4] = {0};
        char name[128] = {0};
        if (sscanf(line, "%lx %3s %127s", &addr, type, name) != 3)
            continue;
        lines_read++;
        if (addr == 0UL) {
            zero_addrs++;
        } else {
            real_addrs++;
            if (!sample_sym) {
                sample_sym  = strdup(name);
                sample_addr = addr;
            }
        }
    }
    fclose(f);

    printf("  Scanned %d entries from /proc/kallsyms\n", lines_read);

    if (real_addrs > 0) {
        printf("  " COL_RED "△" COL_RESET " %d real addresses visible "
               "(running as root or kptr_restrict<2)\n", real_addrs);
        if (sample_sym)
            printf("    Example: %s = 0x%lx\n", sample_sym, sample_addr);
        printf("    This is the info attackers need for KASLR bypass.\n");
        printf("    Set kptr_restrict=2 to zero these out for all users.\n");
    } else {
        printf("  " COL_GREEN "✓" COL_RESET " All addresses show as 0x0000000000000000\n");
        printf("    kptr_restrict=2 is effective — KASLR entropy preserved\n");
    }

    free((void *)sample_sym);
}

static void probe_dmesg(void)
{
    printf("\n  " COL_BOLD "dmesg_restrict probe:" COL_RESET "\n");

    char buf[32] = {0};
    int fd = open("/proc/sys/kernel/dmesg_restrict", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        printf("  Cannot read dmesg_restrict: %s\n", strerror(errno));
        return;
    }
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n > 0) buf[n] = '\0';

    int val = atoi(buf);
    if (val == 1) {
        printf("  " COL_GREEN "✓" COL_RESET " dmesg_restrict=1: "
               "non-root cannot read kernel ring buffer\n");
        printf("    Prevents: kernel address leaks via BUG() traces in dmesg\n");
    } else {
        printf("  " COL_YELLOW "△" COL_RESET " dmesg_restrict=0: "
               "any user can read kernel log\n");
        printf("    Risk: BUG() messages contain raw kernel addresses\n");
    }
}

static void probe_entropy(void)
{
    printf("\n  " COL_BOLD "ASLR entropy (mmap_rnd_bits):" COL_RESET "\n");
    char buf[32] = {0};
    int fd = open("/proc/sys/vm/mmap_rnd_bits", O_RDONLY | O_CLOEXEC);
    if (fd < 0) { printf("  (unavailable)\n"); return; }
    ssize_t n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (n > 0) buf[n] = '\0';

    int bits = atoi(buf);
    unsigned long possibilities = 1UL << bits;
    printf("  mmap_rnd_bits = %d → %lu possible base addresses\n",
           bits, possibilities);
    printf("  At 1 million guesses/sec, brute force takes %.0f years\n",
           (double)possibilities / (1e6 * 3.15e7));

    if (bits >= 32) {
        printf("  " COL_GREEN "✓" COL_RESET " High entropy — brute-force infeasible\n");
    } else if (bits >= 28) {
        printf("  " COL_GREEN "✓" COL_RESET " Good entropy\n");
    } else {
        printf("  " COL_YELLOW "△" COL_RESET " Consider increasing mmap_rnd_bits\n");
    }
}

int main(void)
{
    struct utsname uts;
    uname(&uts);
    printf(COL_BOLD COL_CYAN "\n  KASLR Information Leak Mitigation Probe\n"
           "  Kernel: %s %s\n" COL_RESET, uts.sysname, uts.release);

    probe_kallsyms();
    probe_dmesg();
    probe_entropy();

    printf("\n  " COL_BOLD "Attack chain this stops:" COL_RESET "\n");
    printf("  1. Find kernel vulnerability (UAF, type confusion)\n");
    printf("  2. Need kernel base address to compute gadget offsets\n");
    printf("  3. OLD: /proc/kallsyms or dmesg BUG() leak the address\n");
    printf("  4. NEW: kptr_restrict=2 + dmesg_restrict=1 = zero useful info\n");
    printf("  5. Attacker must use side-channel (cache timing, Spectre) instead\n");
    printf("     — more complex, more likely to be caught, more targets missed\n\n");

    return EXIT_SUCCESS;
}
