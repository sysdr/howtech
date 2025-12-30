#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

// Global connection pool structure
#define MAX_CONNECTIONS 10000
typedef struct {
    int fd;
    int state;  // 0=closed, 1=connecting, 2=established, 3=close_wait
    time_t last_used;
    char remote_addr[64];
} Connection;

typedef struct {
    Connection *connections;
    size_t size;
    size_t capacity;
    pthread_mutex_t lock;
} ConnectionPool;

ConnectionPool *global_conn_pool = NULL;

// Simulate a connection leak scenario
void* connection_leak_thread(void *arg) {
    int thread_id = *(int*)arg;
    (void)thread_id; // Suppress unused variable warning
    free(arg);
    
    for (int i = 0; i < 100; i++) {
        if (global_conn_pool->size < global_conn_pool->capacity) {
            pthread_mutex_lock(&global_conn_pool->lock);
            Connection *conn = &global_conn_pool->connections[global_conn_pool->size];
            conn->fd = 100 + (int)global_conn_pool->size;
            conn->state = 3; // CLOSE_WAIT - the leak!
            conn->last_used = time(NULL);
            snprintf(conn->remote_addr, sizeof(conn->remote_addr), 
                    "192.168.1.%d:%d", (int)((global_conn_pool->size % 254) + 1), 
                    (int)(8000 + (global_conn_pool->size % 1000)));
            global_conn_pool->size++;
            pthread_mutex_unlock(&global_conn_pool->lock);
        }
        usleep(1000); // 1ms
    }
    return NULL;
}

void setup_connection_pool() {
    global_conn_pool = malloc(sizeof(ConnectionPool));
    global_conn_pool->capacity = MAX_CONNECTIONS;
    global_conn_pool->size = 0;
    global_conn_pool->connections = calloc(MAX_CONNECTIONS, sizeof(Connection));
    pthread_mutex_init(&global_conn_pool->lock, NULL);
    
    printf("[+] Connection pool initialized: capacity=%zu\n", global_conn_pool->capacity);
}

void print_system_info() {
    printf("\n=== System Information ===\n");
    printf("PID: %d\n", getpid());
    printf("PPID: %d\n", getppid());
    
    struct rlimit rl;
    getrlimit(RLIMIT_CORE, &rl);
    printf("RLIMIT_CORE: ");
    if (rl.rlim_cur == RLIM_INFINITY)
        printf("unlimited\n");
    else
        printf("%lu bytes\n", rl.rlim_cur);
    
    // Show coredump_filter
    char filter_path[256];
    snprintf(filter_path, sizeof(filter_path), "/proc/%d/coredump_filter", getpid());
    FILE *f = fopen(filter_path, "r");
    if (f) {
        unsigned int filter;
        if (fscanf(f, "%x", &filter) == 1) {
            printf("coredump_filter: 0x%02x\n", filter);
            printf("  - Anonymous private: %s\n", (filter & 0x01) ? "YES" : "NO");
            printf("  - Anonymous shared: %s\n", (filter & 0x02) ? "YES" : "NO");
            printf("  - File-backed private: %s\n", (filter & 0x04) ? "YES" : "NO");
            printf("  - File-backed shared: %s\n", (filter & 0x08) ? "YES" : "NO");
        }
        fclose(f);
    }
    printf("==========================\n\n");
}

void cause_segfault() {
    printf("[!] Triggering NULL pointer dereference in 3 seconds...\n");
    sleep(1);
    printf("[!] 2...\n");
    sleep(1);
    printf("[!] 1...\n");
    sleep(1);
    printf("[!] CRASH!\n");
    
    // Dereference NULL pointer
    int *ptr = NULL;
    *ptr = 42;  // SIGSEGV
}

void cause_abort() {
    printf("[!] Triggering abort() in 3 seconds...\n");
    sleep(1);
    printf("[!] 2...\n");
    sleep(1);
    printf("[!] 1...\n");
    sleep(1);
    printf("[!] CRASH!\n");
    
    abort();  // SIGABRT
}

void cause_fpe() {
    printf("[!] Triggering floating point exception in 3 seconds...\n");
    sleep(1);
    printf("[!] 2...\n");
    sleep(1);
    printf("[!] 1...\n");
    sleep(1);
    printf("[!] CRASH!\n");
    
    volatile int x = 1;
    volatile int y = 0;
    volatile int z = x / y;  // SIGFPE
    (void)z; // Suppress unused warning
}

int main(int argc, char *argv[]) {
    printf("Core Dump Test Program\n");
    printf("======================\n\n");
    
    // Set up unlimited core dumps
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        perror("setrlimit");
        fprintf(stderr, "Warning: Could not set unlimited core size\n");
    }
    
    // Set coredump_filter to include anonymous memory but exclude file-backed
    FILE *f = fopen("/proc/self/coredump_filter", "w");
    if (f) {
        fprintf(f, "0x33");  // anonymous private + anonymous shared + file-private
        fclose(f);
    }
    
    print_system_info();
    
    // Set up connection pool to make debugging interesting
    setup_connection_pool();
    
    // Spawn threads that create connection leaks
    printf("[+] Starting connection leak simulation...\n");
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&threads[i], NULL, connection_leak_thread, thread_id);
    }
    
    // Wait for leaks to accumulate
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("[+] Connection leak complete: %zu connections in CLOSE_WAIT\n", 
           global_conn_pool->size);
    
    // Allocate some heap memory to make the dump interesting
    size_t heap_size = 64 * 1024 * 1024;  // 64MB
    char *heap_data = malloc(heap_size);
    if (heap_data) {
        memset(heap_data, 0xAA, heap_size);
        printf("[+] Allocated %zu MB heap memory\n", heap_size / (1024*1024));
    }
    
    // Choose crash type
    int crash_type = 1; // Default to segfault
    if (argc > 1) {
        crash_type = atoi(argv[1]);
    }
    
    printf("\nCrash type: ");
    switch (crash_type) {
        case 1:
            printf("NULL pointer dereference (SIGSEGV)\n");
            cause_segfault();
            break;
        case 2:
            printf("Abort (SIGABRT)\n");
            cause_abort();
            break;
        case 3:
            printf("Division by zero (SIGFPE)\n");
            cause_fpe();
            break;
        default:
            printf("Unknown, using segfault\n");
            cause_segfault();
            break;
    }
    
    return 0;
}
