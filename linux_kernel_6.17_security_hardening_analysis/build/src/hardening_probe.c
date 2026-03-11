/*
 * hardening_probe.c — Read kernel security feature state from /proc and /sys
 * Demonstrates observable evidence of 6.17 hardening features at runtime.
 *
 * Build: gcc -Wall -Wextra -Werror -O2 -D_FORTIFY_SOURCE=3
 *             -fstack-protector-strong -pie -fPIE -o hardening_probe hardening_probe.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>

#define KNOB_GREEN  "\033[0;32m"
#define KNOB_RED    "\033[0;31m"
#define KNOB_YELLOW "\033[1;33m"
#define KNOB_RESET  "\033[0m"
#define KNOB_BOLD   "\033[1m"
#define KNOB_CYAN   "\033[0;36m"

typedef struct {
    const char *label;
    const char *path;
    const char *good_val;   /* NULL means "file present = good" */
    const char *description;
} security_knob_t;

static const security_knob_t knobs[] = {
    /* Kernel pointer restriction */
    { "kptr_restrict",        "/proc/sys/kernel/kptr_restrict",          "2",
      "Kernel pointers hidden from /proc (kASLR protection)" },
    /* dmesg access control */
    { "dmesg_restrict",       "/proc/sys/kernel/dmesg_restrict",         "1",
      "Non-root cannot read kernel log (info leak mitigation)" },
    /* ASLR level */
    { "randomize_va_space",   "/proc/sys/kernel/randomize_va_space",     "2",
      "Full ASLR: stack, VDSO, mmap, brk all randomized" },
    /* Perf paranoia */
    { "perf_event_paranoid",  "/proc/sys/kernel/perf_event_paranoid",    "3",
      "Unprivileged perf blocked (KASLR brute-force protection)" },
    /* Unprivileged BPF */
    { "unprivileged_bpf_disabled", "/proc/sys/kernel/unprivileged_bpf_disabled", "1",
      "Unprivileged BPF disabled (verifier bug mitigation)" },
    /* io_uring restriction */
    { "io_uring_disabled",    "/proc/sys/kernel/io_uring_disabled",      "0",
      "io_uring enabled; 1=restricted, 2=disabled" },
    /* Core pattern (prevents setuid exploit via core) */
    { "core_uses_pid",        "/proc/sys/kernel/core_uses_pid",          "1",
      "Unique core files per PID (race condition mitigation)" },
    /* YAMA ptrace scope */
    { "yama_ptrace_scope",    "/proc/sys/kernel/yama/ptrace_scope",      "1",
      "YAMA: only parent can ptrace child processes" },
    /* Hardened usercopy */
    { "hardened_usercopy",    "/sys/module/kernel/parameters/hardened_usercopy", NULL,
      "Bounds-checked user↔kernel copies (vmalloc range 6.17)" },
    { NULL, NULL, NULL, NULL }
};

static int read_sysctl(const char *path, char *buf, size_t len)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -errno;
    ssize_t n = read(fd, buf, len - 1);
    close(fd);
    if (n < 0)
        return -errno;
    buf[n] = '\0';
    /* strip newline */
    char *nl = strchr(buf, '\n');
    if (nl)
        *nl = '\0';
    return 0;
}

static void print_banner(void)
{
    struct utsname uts;
    if (uname(&uts) == 0) {
        printf(KNOB_BOLD KNOB_CYAN
               "\n  Kernel Security Feature Probe\n"
               "  Kernel: %s %s\n"
               "  Arch:   %s\n\n" KNOB_RESET,
               uts.sysname, uts.release, uts.machine);
    }
    printf("  %-30s  %-10s  %s\n", "Feature", "Value", "Status");
    printf("  %s\n", "────────────────────────────────────────────────────────────────────");
}

static void probe_aslr_entropy(void)
{
    /* On x86-64, read ASLR entropy from /proc/sys/vm/mmap_rnd_bits */
    char buf[64] = {0};
    if (read_sysctl("/proc/sys/vm/mmap_rnd_bits", buf, sizeof(buf)) == 0) {
        int bits = atoi(buf);
        const char *color = (bits >= 28) ? KNOB_GREEN : KNOB_YELLOW;
        printf("  %-30s  %-10s  %s%s%s\n",
               "mmap_rnd_bits (ASLR entropy)", buf,
               color,
               bits >= 28 ? "✓ High entropy" : "△ Moderate entropy",
               KNOB_RESET);
        printf("    %s→%s %d-bit entropy = %lu possible base addresses\n",
               KNOB_YELLOW, KNOB_RESET, bits, 1UL << bits);
    }
}

static void probe_stack_canary(void)
{
    /*
     * Stack canary is not directly observable in /proc, but we can verify
     * -fstack-protector-strong is active by checking our own binary's
     * __stack_chk_fail PLT entry is present. Here we just report build flags.
     */
    printf("\n  " KNOB_BOLD "Stack Protection (this binary):" KNOB_RESET "\n");
#ifdef __SSP_STRONG__
    printf("  %-30s  %-10s  " KNOB_GREEN "✓ Active (STRONG)" KNOB_RESET "\n",
           "fstack-protector-strong", "compiled");
#elif defined(__SSP__)
    printf("  %-30s  %-10s  " KNOB_YELLOW "△ Basic only" KNOB_RESET "\n",
           "fstack-protector", "compiled");
#else
    printf("  %-30s  %-10s  " KNOB_RED "✗ Disabled" KNOB_RESET "\n",
           "stack-protector", "absent");
#endif

#if defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE >= 3
    printf("  %-30s  %-10d  " KNOB_GREEN "✓ Level 3 (6.17 default)" KNOB_RESET "\n",
           "_FORTIFY_SOURCE", _FORTIFY_SOURCE);
#elif defined(_FORTIFY_SOURCE) && _FORTIFY_SOURCE == 2
    printf("  %-30s  %-10d  " KNOB_YELLOW "△ Level 2 (pre-6.17)" KNOB_RESET "\n",
           "_FORTIFY_SOURCE", _FORTIFY_SOURCE);
#else
    printf("  %-30s  %-10s  " KNOB_RED "✗ Disabled" KNOB_RESET "\n",
           "_FORTIFY_SOURCE", "0");
#endif
}

static void probe_pie(void)
{
    printf("\n  " KNOB_BOLD "Position Independent Executable:" KNOB_RESET "\n");
    /* Read our own /proc/self/maps to see if text segment is not at 0x400000 */
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) {
        printf("  Cannot read /proc/self/maps: %s\n", strerror(errno));
        return;
    }
    char line[256];
    int found_text = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "r-xp") && !found_text) {
            unsigned long start = 0;
            sscanf(line, "%lx-", &start);
            found_text = 1;
            if (start < 0x100000000UL && start != 0x400000UL) {
                printf("  %-30s  0x%lx  " KNOB_GREEN "✓ PIE active (ASLR applied)" KNOB_RESET "\n",
                       "text segment base", start);
            } else if (start == 0x400000UL) {
                printf("  %-30s  0x%lx  " KNOB_RED "✗ Non-PIE (fixed base)" KNOB_RESET "\n",
                       "text segment base", start);
            } else {
                printf("  %-30s  0x%lx  " KNOB_GREEN "✓ PIE active" KNOB_RESET "\n",
                       "text segment base", start);
            }
        }
    }
    fclose(f);
}

int main(void)
{
    print_banner();

    for (const security_knob_t *k = knobs; k->label != NULL; k++) {
        char buf[128] = "(not found)";
        int rc = read_sysctl(k->path, buf, sizeof(buf));
        const char *status_color;
        const char *status_text;

        if (rc != 0) {
            status_color = KNOB_YELLOW;
            status_text  = "? (unavailable)";
        } else if (k->good_val == NULL) {
            status_color = KNOB_GREEN;
            status_text  = "✓ Present";
        } else if (strcmp(buf, k->good_val) == 0) {
            status_color = KNOB_GREEN;
            status_text  = "✓ Hardened";
        } else {
            status_color = KNOB_YELLOW;
            status_text  = "△ Suboptimal";
        }

        printf("  %-30s  %-10s  %s%s%s\n",
               k->label, buf, status_color, status_text, KNOB_RESET);
        printf("    %s→%s %s\n", KNOB_YELLOW, KNOB_RESET, k->description);
    }

    probe_aslr_entropy();
    probe_stack_canary();
    probe_pie();

    printf("\n");
    return EXIT_SUCCESS;
}
