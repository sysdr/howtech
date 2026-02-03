#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#define NSEC_PER_SEC 1000000000LL
#define HISTOGRAM_BUCKETS 200
#define TEST_DURATION_SEC 10
#define LOOP_INTERVAL_US 1000

// ARM cycle counter access (if available)
static inline uint64_t read_cycles(void) {
#ifdef __aarch64__
    uint64_t cycles;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r" (cycles));
    return cycles;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

static inline uint64_t timespec_to_ns(const struct timespec *ts) {
    return (uint64_t)ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static inline void timespec_add_us(struct timespec *ts, long us) {
    ts->tv_nsec += us * 1000;
    while (ts->tv_nsec >= NSEC_PER_SEC) {
        ts->tv_nsec -= NSEC_PER_SEC;
        ts->tv_sec++;
    }
}

typedef struct {
    uint64_t histogram[HISTOGRAM_BUCKETS];
    uint64_t min_latency;
    uint64_t max_latency;
    uint64_t total_samples;
    uint64_t overflow_count;
    pthread_mutex_t lock;
} latency_stats_t;

static latency_stats_t stats = {
    .min_latency = UINT64_MAX,
    .max_latency = 0,
    .total_samples = 0,
    .overflow_count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

void update_stats(uint64_t latency_ns) {
    pthread_mutex_lock(&stats.lock);
    
    stats.total_samples++;
    if (latency_ns < stats.min_latency) stats.min_latency = latency_ns;
    if (latency_ns > stats.max_latency) stats.max_latency = latency_ns;
    
    // Histogram with 1us buckets
    uint64_t bucket = latency_ns / 1000;
    if (bucket < HISTOGRAM_BUCKETS) {
        stats.histogram[bucket]++;
    } else {
        stats.overflow_count++;
    }
    
    pthread_mutex_unlock(&stats.lock);
}

void* latency_thread(void* arg) {
    int cpu = *(int*)arg;
    cpu_set_t cpuset;
    struct sched_param param;
    struct timespec next, now;
    
    // Pin to specific CPU
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        return NULL;
    }
    
    // Set real-time priority
    param.sched_priority = 95;
    if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0) {
        fprintf(stderr, "Warning: Cannot set RT priority (need CAP_SYS_NICE)\n");
    }
    
    // Get current time
    clock_gettime(CLOCK_MONOTONIC, &next);
    timespec_add_us(&next, LOOP_INTERVAL_US);
    
    printf("Latency test thread running on CPU %d (PID: %ld)\n", 
           cpu, (long)syscall(SYS_gettid));
    
    // Run test loop
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < TEST_DURATION_SEC) {
        // Sleep until next interval
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) != 0) {
            perror("clock_nanosleep");
            break;
        }
        
        // Measure actual wakeup time
        clock_gettime(CLOCK_MONOTONIC, &now);
        
        // Calculate latency
        uint64_t expected_ns = timespec_to_ns(&next);
        uint64_t actual_ns = timespec_to_ns(&now);
        uint64_t latency_ns = (actual_ns > expected_ns) ? 
                               (actual_ns - expected_ns) : 0;
        
        update_stats(latency_ns);
        
        // Setup next interval
        timespec_add_us(&next, LOOP_INTERVAL_US);
    }
    
    return NULL;
}

void print_histogram(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           Latency Histogram (bucket = 1μs)                   ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    uint64_t max_count = 0;
    for (int i = 0; i < HISTOGRAM_BUCKETS; i++) {
        if (stats.histogram[i] > max_count) max_count = stats.histogram[i];
    }
    
    // Find significant range
    int start = 0, end = HISTOGRAM_BUCKETS - 1;
    for (int i = 0; i < HISTOGRAM_BUCKETS; i++) {
        if (stats.histogram[i] > 0) {
            start = i;
            break;
        }
    }
    for (int i = HISTOGRAM_BUCKETS - 1; i >= 0; i--) {
        if (stats.histogram[i] > 0) {
            end = i;
            break;
        }
    }
    
    // Print histogram bars
    for (int i = start; i <= end && i < HISTOGRAM_BUCKETS; i++) {
        if (stats.histogram[i] == 0) continue;
        
        printf("║ %3dμs: ", i);
        int bar_len = (int)((stats.histogram[i] * 40.0) / max_count);
        for (int j = 0; j < bar_len; j++) printf("█");
        printf(" %lu\n", stats.histogram[i]);
    }
    
    if (stats.overflow_count > 0) {
        printf("║ >%dμs: %lu samples (overflow)\n", 
               HISTOGRAM_BUCKETS - 1, stats.overflow_count);
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}

void print_statistics(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                     Latency Statistics                        ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ Total Samples:  %10lu                                   ║\n", stats.total_samples);
    printf("║ Min Latency:    %10.2f μs                              ║\n", stats.min_latency / 1000.0);
    printf("║ Max Latency:    %10.2f μs                              ║\n", stats.max_latency / 1000.0);
    
    // Calculate percentiles
    uint64_t counts[3] = {0, 0, 0}; // 50th, 95th, 99th
    uint64_t targets[3] = {
        stats.total_samples / 2,
        stats.total_samples * 95 / 100,
        stats.total_samples * 99 / 100
    };
    int found[3] = {0, 0, 0};
    uint64_t running = 0;
    
    for (int i = 0; i < HISTOGRAM_BUCKETS && !found[2]; i++) {
        running += stats.histogram[i];
        for (int j = 0; j < 3; j++) {
            if (!found[j] && running >= targets[j]) {
                counts[j] = i;
                found[j] = 1;
            }
        }
    }
    
    printf("║ 50th percentile: %9.2f μs                              ║\n", counts[0] * 1.0);
    printf("║ 95th percentile: %9.2f μs                              ║\n", counts[1] * 1.0);
    printf("║ 99th percentile: %9.2f μs                              ║\n", counts[2] * 1.0);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char **argv) {
    int test_cpu = 0;
    
    if (argc > 1) {
        test_cpu = atoi(argv[1]);
    }
    
    // Lock memory to prevent page faults
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        fprintf(stderr, "Warning: Cannot lock memory (need CAP_IPC_LOCK)\n");
    }
    
    printf("\n");
    printf("ARM Real-Time Latency Test\n");
    printf("==========================\n");
#ifdef __aarch64__
    printf("Architecture: ARM64\n");
#elif defined(__arm__)
    printf("Architecture: ARM32\n");
#else
    printf("Architecture: %s\n", "x86_64 (limited ARM features)");
#endif
    printf("Test CPU: %d\n", test_cpu);
    printf("Interval: %d μs\n", LOOP_INTERVAL_US);
    printf("Duration: %d seconds\n", TEST_DURATION_SEC);
    printf("\n");
    
    // Run latency test
    pthread_t thread;
    if (pthread_create(&thread, NULL, latency_thread, &test_cpu) != 0) {
        perror("pthread_create");
        return 1;
    }
    
    pthread_join(thread, NULL);
    
    // Print results
    print_statistics();
    print_histogram();
    
    printf("\nTest complete. Results saved to results/latency_stats.txt\n");
    
    // Save results to file
    FILE *fp = fopen("results/latency_stats.txt", "w");
    if (fp) {
        fprintf(fp, "Min: %.2f μs\n", stats.min_latency / 1000.0);
        fprintf(fp, "Max: %.2f μs\n", stats.max_latency / 1000.0);
        fprintf(fp, "Samples: %lu\n", stats.total_samples);
        fclose(fp);
    }
    
    return 0;
}
