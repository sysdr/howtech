#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void maybe_pin_to_cpu(int cpu, int ncpus)
{
    if (cpu < 0 || cpu >= ncpus)
        return;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        fprintf(stderr, "warn: sched_setaffinity(cpu=%d) failed: %s\n", cpu,
                strerror(errno));
    }
}

int main(int argc, char **argv)
{
    int ncpus = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus <= 0)
        ncpus = 1;

    int hops = (ncpus >= 4) ? 4 : ncpus;
    int iterations = 8;

    if (argc > 1) {
        int v = atoi(argv[1]);
        if (v > 0)
            iterations = v;
    }

    printf("scx_task_migration: userspace simulation\n");
    printf("  cpus_online=%d  hops=%d  iterations=%d\n\n", ncpus, hops,
           iterations);

    long long t0 = now_ns();
    for (int it = 0; it < iterations; it++) {
        int cpu = it % hops;
        maybe_pin_to_cpu(cpu, ncpus);

        long long a = now_ns();
        usleep(80 * 1000);
        long long b = now_ns();

        printf("  iter=%2d  pinned_cpu=%d  slice_ms=%.2f\n", it + 1, cpu,
               (double)(b - a) / 1e6);
        fflush(stdout);
    }

    long long t1 = now_ns();
    printf("\n  total_ms=%.2f\n", (double)(t1 - t0) / 1e6);
    printf("scx_task_migration: done\n");
    return 0;
}
