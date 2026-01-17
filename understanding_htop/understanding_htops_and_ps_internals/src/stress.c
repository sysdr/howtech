#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

static int running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

void *cpu_burn_thread(void *arg) {
    (void)arg;
    unsigned long long counter = 0;
    
    while (running) {
        counter++;
        if (counter % 100000000 == 0) {
            /* Occasional syscall to show user vs sys time */
            getpid();
        }
    }
    
    return NULL;
}

void *io_wait_thread(void *arg) {
    (void)arg;
    
    while (running) {
        sleep(1);
    }
    
    return NULL;
}

void create_zombie(void) {
    pid_t child = fork();
    if (child == 0) {
        /* Child exits immediately */
        exit(0);
    }
    /* Parent doesn't wait - creates zombie */
}

int main(int argc, char *argv[]) {
    int num_cpu_threads = 2;
    int num_io_threads = 2;
    int create_zombies = 0;
    
    if (argc > 1) {
        num_cpu_threads = atoi(argv[1]);
    }
    if (argc > 2) {
        num_io_threads = atoi(argv[2]);
    }
    if (argc > 3) {
        create_zombies = atoi(argv[3]);
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Stress test starting...\n");
    printf("  CPU threads: %d\n", num_cpu_threads);
    printf("  I/O threads: %d\n", num_io_threads);
    printf("  Zombies: %d\n", create_zombies);
    printf("\n");
    printf("PID: %d\n", getpid());
    printf("Press Ctrl+C to stop\n\n");
    
    /* Create CPU-intensive threads */
    pthread_t *cpu_threads = malloc(num_cpu_threads * sizeof(pthread_t));
    for (int i = 0; i < num_cpu_threads; i++) {
        pthread_create(&cpu_threads[i], NULL, cpu_burn_thread, NULL);
    }
    
    /* Create I/O waiting threads */
    pthread_t *io_threads = malloc(num_io_threads * sizeof(pthread_t));
    for (int i = 0; i < num_io_threads; i++) {
        pthread_create(&io_threads[i], NULL, io_wait_thread, NULL);
    }
    
    /* Create zombie processes */
    for (int i = 0; i < create_zombies; i++) {
        create_zombie();
    }
    
    /* Wait for signal */
    while (running) {
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    
    /* Wait for threads */
    for (int i = 0; i < num_cpu_threads; i++) {
        pthread_join(cpu_threads[i], NULL);
    }
    for (int i = 0; i < num_io_threads; i++) {
        pthread_join(io_threads[i], NULL);
    }
    
    /* Reap zombies */
    while (waitpid(-1, NULL, WNOHANG) > 0);
    
    free(cpu_threads);
    free(io_threads);
    
    return 0;
}
