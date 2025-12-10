#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MSG_SIZE 64
#define NUM_MESSAGES 1000000
#define QUEUE_SIZE 10
#define MQ_NAME "/ipc_benchmark_mq"

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void producer(mqd_t mq) {
    char msg[MSG_SIZE];
    snprintf(msg, MSG_SIZE, "Message");
    
    uint64_t start = rdtsc();
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (mq_send(mq, msg, MSG_SIZE, 0) == -1) {
            perror("mq_send");
            exit(1);
        }
    }
    
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;
    
    printf("[Producer] Sent %d messages\n", NUM_MESSAGES);
    printf("[Producer] Total cycles: %lu\n", cycles);
    printf("[Producer] Cycles per message: %lu\n", cycles / NUM_MESSAGES);
}

void consumer(mqd_t mq) {
    char msg[MSG_SIZE];
    
    uint64_t start = rdtsc();
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (mq_receive(mq, msg, MSG_SIZE, NULL) == -1) {
            perror("mq_receive");
            exit(1);
        }
    }
    
    uint64_t end = rdtsc();
    uint64_t cycles = end - start;
    
    printf("[Consumer] Received %d messages\n", NUM_MESSAGES);
    printf("[Consumer] Total cycles: %lu\n", cycles);
    printf("[Consumer] Cycles per message: %lu\n", cycles / NUM_MESSAGES);
}

int main(void) {
    printf("\n=== POSIX Message Queue Benchmark ===\n\n");
    
    // Unlink any existing queue
    mq_unlink(MQ_NAME);
    
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = QUEUE_SIZE,
        .mq_msgsize = MSG_SIZE,
        .mq_curmsgs = 0
    };
    
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0600, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    
    if (pid == 0) {
        // Child: Consumer
        consumer(mq);
        mq_close(mq);
        exit(0);
    } else {
        // Parent: Producer
        producer(mq);
        
        int status;
        waitpid(pid, &status, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        double throughput = NUM_MESSAGES / elapsed;
        
        printf("\nTotal time: %.3f seconds\n", elapsed);
        printf("Throughput: %.0f messages/sec\n", throughput);
        printf("Average latency: %.0f ns/message\n", (elapsed * 1e9) / NUM_MESSAGES);
        
        mq_close(mq);
        mq_unlink(MQ_NAME);
    }
    
    return 0;
}
