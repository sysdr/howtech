#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#define RING_SIZE 1024
#define MSG_SIZE 64
#define NUM_MESSAGES 1000000
#define SHM_NAME "/ipc_benchmark_shm"

// Cache line aligned structure to prevent false sharing
struct ring_buffer {
    atomic_uint_fast64_t write_index;
    char _pad1[64 - sizeof(atomic_uint_fast64_t)];
    
    atomic_uint_fast64_t read_index;
    char _pad2[64 - sizeof(atomic_uint_fast64_t)];
    
    char data[RING_SIZE][MSG_SIZE];
} __attribute__((aligned(64)));

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void write_message(struct ring_buffer *rb, const char *msg) {
    while (1) {
        uint64_t w = atomic_load_explicit(&rb->write_index, memory_order_relaxed);
        uint64_t r = atomic_load_explicit(&rb->read_index, memory_order_acquire);
        
        if (w - r >= RING_SIZE) {
            continue; // Buffer full, spin
        }
        
        memcpy(rb->data[w % RING_SIZE], msg, MSG_SIZE);
        atomic_thread_fence(memory_order_release);
        atomic_store_explicit(&rb->write_index, w + 1, memory_order_release);
        return;
    }
}

static inline int read_message(struct ring_buffer *rb, char *msg) {
    while (1) {
        uint64_t r = atomic_load_explicit(&rb->read_index, memory_order_relaxed);
        uint64_t w = atomic_load_explicit(&rb->write_index, memory_order_acquire);
        
        if (r == w) {
            return 0; // Buffer empty
        }
        
        atomic_thread_fence(memory_order_acquire);
        memcpy(msg, rb->data[r % RING_SIZE], MSG_SIZE);
        atomic_store_explicit(&rb->read_index, r + 1, memory_order_release);
        return 1;
    }
}

void producer(struct ring_buffer *rb) {
    char msg[MSG_SIZE];
    snprintf(msg, MSG_SIZE, "Message");
    
    uint64_t start = rdtsc();
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        write_message(rb, msg);
    }
    
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;
    
    printf("[Producer] Sent %d messages\n", NUM_MESSAGES);
    printf("[Producer] Total cycles: %lu\n", cycles);
    printf("[Producer] Cycles per message: %lu\n", cycles / NUM_MESSAGES);
}

void consumer(struct ring_buffer *rb) {
    char msg[MSG_SIZE];
    int count = 0;
    
    uint64_t start = rdtsc();
    
    while (count < NUM_MESSAGES) {
        if (read_message(rb, msg)) {
            count++;
        }
    }
    
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;
    
    printf("[Consumer] Received %d messages\n", count);
    printf("[Consumer] Total cycles: %lu\n", cycles);
    printf("[Consumer] Cycles per message: %lu\n", cycles / NUM_MESSAGES);
}

int main(void) {
    printf("\n=== Shared Memory Ring Buffer Benchmark ===\n\n");
    
    // Create shared memory
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    if (ftruncate(fd, sizeof(struct ring_buffer)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        return 1;
    }
    
    struct ring_buffer *rb = mmap(NULL, sizeof(struct ring_buffer),
                                   PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (rb == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return 1;
    }
    
    // Initialize
    atomic_init(&rb->write_index, 0);
    atomic_init(&rb->read_index, 0);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child: Consumer
        consumer(rb);
        exit(0);
    } else {
        // Parent: Producer
        producer(rb);
        
        int status;
        waitpid(pid, &status, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        double throughput = NUM_MESSAGES / elapsed;
        
        printf("\nTotal time: %.3f seconds\n", elapsed);
        printf("Throughput: %.0f messages/sec\n", throughput);
        printf("Average latency: %.0f ns/message\n", (elapsed * 1e9) / NUM_MESSAGES);
    }
    
    munmap(rb, sizeof(struct ring_buffer));
    close(fd);
    shm_unlink(SHM_NAME);
    
    return 0;
}
