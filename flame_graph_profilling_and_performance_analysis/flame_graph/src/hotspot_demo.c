#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// Intentional hotspot: inefficient JSON-like parsing
char* parse_json_slow(const char* input) {
    size_t len = strlen(input);
    char* result = malloc(len * 2);
    if (!result) return NULL;
    
    int pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (input[i] == '{' || input[i] == '}' || input[i] == '"') {
            result[pos++] = '\\';
        }
        result[pos++] = input[i];
    }
    result[pos] = '\0';
    return result;
}

// Intentional hotspot: excessive malloc/free
void* allocate_and_copy(const char* str) {
    void* ptr = malloc(strlen(str) + 1);
    if (ptr) {
        strcpy(ptr, str);
    }
    return ptr;
}

void process_data(const char* data) {
    // This simulates JSON parsing workload
    for (int i = 0; i < 1000; i++) {
        char* parsed = parse_json_slow(data);
        if (parsed) {
            // More allocations
            char* copy1 = allocate_and_copy(parsed);
            char* copy2 = allocate_and_copy(parsed);
            char* copy3 = allocate_and_copy(parsed);
            
            free(copy3);
            free(copy2);
            free(copy1);
            free(parsed);
        }
    }
}

// CPU-intensive computation
double compute_intensive() {
    double result = 0.0;
    for (int i = 0; i < 1000000; i++) {
        result += i * 0.001;
    }
    return result;
}

// I/O simulation
void io_simulation() {
    char buffer[4096];
    FILE* fp = fopen("/dev/urandom", "r");
    if (fp) {
        for (int i = 0; i < 100; i++) {
            fread(buffer, 1, sizeof(buffer), fp);
        }
        fclose(fp);
    }
}

void request_handler() {
    const char* test_data = "{\"user\":\"test\",\"action\":\"parse\",\"data\":\"sample\"}";
    
    // Mix of different workloads
    process_data(test_data);  // Heavy malloc/free
    compute_intensive();       // CPU bound
    io_simulation();          // I/O bound
}

int main(int argc, char** argv) {
    printf("Flame Graph Demo: Running workload with hotspots...\n");
    printf("PID: %d\n", getpid());
    printf("Running for 30 seconds...\n\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int iterations = 0;
    while (1) {
        request_handler();
        iterations++;
        
        gettimeofday(&end, NULL);
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_usec - start.tv_usec) / 1000000.0;
        
        if (elapsed >= 30.0) {
            break;
        }
        
        if (iterations % 10 == 0) {
            printf("\rProcessed %d requests (%.1f seconds)...", iterations, elapsed);
            fflush(stdout);
        }
    }
    
    printf("\nCompleted %d requests in 30 seconds\n", iterations);
    return 0;
}
