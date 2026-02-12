#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

#define MAX_ALLOCS 1000

typedef struct {
    void *ptr;
    size_t size;
} Allocation;

void fragment_memory(int iterations) {
    Allocation allocs[MAX_ALLOCS];
    int count = 0;
    
    printf("Fragmenting memory with random allocations...\n");
    
    for (int iter = 0; iter < iterations && count < MAX_ALLOCS; iter++) {
        // Random size between 4KB and 2MB
        size_t size = (4 + (rand() % 508)) * 4096;
        
        void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (ptr != MAP_FAILED) {
            memset(ptr, iter & 0xFF, size);
            allocs[count].ptr = ptr;
            allocs[count].size = size;
            count++;
            
            // Randomly free some allocations
            if (count > 10 && (rand() % 3 == 0)) {
                int idx = rand() % count;
                munmap(allocs[idx].ptr, allocs[idx].size);
                // Move last to removed position
                allocs[idx] = allocs[count - 1];
                count--;
            }
        }
        
        if (iter % 100 == 0) {
            printf("  Iteration %d: %d active allocations\n", iter, count);
        }
    }
    
    printf("Fragmentation complete. %d allocations remain.\n", count);
    printf("Check /proc/buddyinfo to see fragmentation.\n");
    
    // Keep allocations for observation
    sleep(5);
    
    // Cleanup
    for (int i = 0; i < count; i++) {
        munmap(allocs[i].ptr, allocs[i].size);
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    int iterations = 500;
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    
    printf("=== Memory Fragmentation Stress Test ===\n");
    printf("This will create fragmentation patterns observable in /proc/buddyinfo\n\n");
    
    fragment_memory(iterations);
    
    return 0;
}
