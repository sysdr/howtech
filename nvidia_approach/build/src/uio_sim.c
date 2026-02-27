/*
 * uio_sim.c — UIO userspace driver simulation
 *
 * Demonstrates the minimal kernel interface a UIO driver uses:
 *   open(2) → mmap(2) → read(2) for IRQ count → close(2)
 *
 * In a real UIO driver the kernel shim is <200 lines. All device logic
 * (register programming, error recovery, DMA management) lives here in
 * userspace and is qualified as an SEooC — independent of the kernel version.
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -o uio_sim uio_sim.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MMIO_SIZE     (4096)
#define IRQ_CYCLES    (8)
#define NS_PER_S      (1000000000LL)

/* Simulated device register offsets */
#define REG_STATUS    (0x00)
#define REG_CTRL      (0x04)
#define REG_DATA      (0x08)
#define REG_IRQ_MASK  (0x0C)
#define REG_VERSION   (0x10)

static inline uint64_t rdtsc_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * NS_PER_S + (uint64_t)ts.tv_nsec;
}

/* In a real UIO driver, this is mmap() of /dev/uioN at offset 0.
 * We allocate an anonymous page to simulate the MMIO window. */
static uint32_t *open_mmio_region(void) {
    void *ptr = mmap(NULL, MMIO_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap MMIO region");
        exit(EXIT_FAILURE);
    }
    /* Simulate device identity registers */
    uint32_t *regs = (uint32_t *)ptr;
    regs[REG_VERSION / 4] = 0xA000CAFE;
    regs[REG_STATUS  / 4] = 0x00000001; /* device ready */
    regs[REG_IRQ_MASK/ 4] = 0x000000FF;
    return regs;
}

/* UIO interrupt handling: read(uio_fd) returns 4-byte interrupt count.
 * We simulate this with a pipe. Real UIO uses /dev/uioN. */
static void simulate_uio_irq_cycle(int irq_pipe_rd, int irq_pipe_wr,
                                   uint32_t *regs, int cycle) {
    uint64_t t0, t1;
    uint32_t irq_count = 0;
    ssize_t r;

    /* Mask the interrupt (write REG_CTRL) */
    regs[REG_CTRL / 4] = 0x00000000;

    /* Simulate device raising IRQ — in real UIO the kernel delivers this */
    uint32_t count_val = (uint32_t)(cycle + 1);
    if (write(irq_pipe_wr, &count_val, sizeof(count_val)) != sizeof(count_val)) {
        perror("write irq pipe");
        exit(EXIT_FAILURE);
    }

    t0 = rdtsc_ns();

    /* UIO IRQ path: read(fd) returns when interrupt fires */
    r = read(irq_pipe_rd, &irq_count, sizeof(irq_count));
    if (r != (ssize_t)sizeof(irq_count)) {
        perror("read UIO irq count");
        exit(EXIT_FAILURE);
    }

    t1 = rdtsc_ns();

    /* Update device DATA register with result */
    regs[REG_DATA / 4] = irq_count * 0x1000 + (uint32_t)(t1 - t0);

    /* Re-enable interrupt (unmask) */
    regs[REG_CTRL / 4] = 0x00000001;

    printf("  IRQ %-3d | count=%-4u | latency=%5llu ns | "
           "STATUS=0x%08X DATA=0x%08X\n",
           cycle, irq_count, (unsigned long long)(t1 - t0),
           regs[REG_STATUS / 4], regs[REG_DATA / 4]);
}

/* Count lines in a file — used to show in-kernel driver size vs UIO shim */
static int count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    int lines = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') lines++;
    }
    fclose(f);
    return lines;
}

int main(void) {
    printf("\n\033[1;34m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;34m║\033[0m  UIO Userspace Driver Simulation                     \033[1;34m║\033[0m\n");
    printf("\033[1;34m║\033[0m  Demonstrating minimal kernel footprint approach      \033[1;34m║\033[0m\n");
    printf("\033[1;34m╚══════════════════════════════════════════════════════╝\033[0m\n\n");

    /* Show our own source size to make the point concrete */
    int our_lines = count_lines("/src/uio_sim.c");
    int kernel_est = 2800; /* typical full in-kernel DRM/V4L2 driver estimate */

    printf("\033[1;33m  Kernel footprint comparison:\033[0m\n");
    printf("  ┌─────────────────────────────────────────────────┐\n");
    printf("  │ In-kernel DRM/V4L2 driver:   ~%4d lines of C   │\n", kernel_est);
    printf("  │ UIO shim (kernel side):        < 200 lines      │\n");
    if (our_lines > 0)
        printf("  │ This userspace driver:         %4d lines       │\n", our_lines);
    printf("  │                                                  │\n");
    printf("  │ Qualification delta on LTS update:               │\n");
    printf("  │   In-kernel: full re-analysis of kernel diff     │\n");
    printf("  │   UIO shim:  ZERO — upstream, no diff to own     │\n");
    printf("  └─────────────────────────────────────────────────┘\n\n");

    /* Open simulated MMIO region */
    uint32_t *regs = open_mmio_region();
    printf("\033[0;32m  MMIO region mapped\033[0m (simulates /dev/uioN mmap at offset 0)\n");
    printf("  Device version register: 0x%08X\n\n", regs[REG_VERSION / 4]);

    /* Create pipe to simulate UIO IRQ delivery */
    int irq_pipe[2];
    if (pipe2(irq_pipe, O_NONBLOCK) == -1) {
        /* fallback to blocking pipe */
        if (pipe(irq_pipe) == -1) {
            perror("pipe");
            munmap(regs, MMIO_SIZE);
            return EXIT_FAILURE;
        }
    }

    printf("  \033[1mSimulating %d UIO interrupt cycles:\033[0m\n", IRQ_CYCLES);
    printf("  %-7s %-12s %-20s %-22s\n",
           "IRQ", "count", "latency", "register state");
    printf("  %s\n", "─────────────────────────────────────────────────────────────");

    for (int i = 0; i < IRQ_CYCLES; i++) {
        simulate_uio_irq_cycle(irq_pipe[0], irq_pipe[1], regs, i);
        usleep(5000);
    }

    close(irq_pipe[0]);
    close(irq_pipe[1]);
    munmap(regs, MMIO_SIZE);

    printf("\n\033[0;32m  ✓ UIO simulation complete — no kernel code modified\033[0m\n");
    printf("  \033[2mAll device logic above runs in userspace. Kernel shim\033[0m\n");
    printf("  \033[2m(uio_pdrv_genirq) is upstream — zero qualification delta\033[0m\n\n");
    return EXIT_SUCCESS;
}
