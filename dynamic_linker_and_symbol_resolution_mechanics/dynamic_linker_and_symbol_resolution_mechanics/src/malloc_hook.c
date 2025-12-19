#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <execinfo.h>
#include <unistd.h>
#include <string.h>

// Statistics tracking
static atomic_size_t total_allocations = 0;
static atomic_size_t total_frees = 0;
static atomic_size_t bytes_allocated = 0;
static atomic_size_t bytes_freed = 0;
static atomic_size_t current_usage = 0;
static atomic_size_t peak_usage = 0;

// Thread-safety
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Prevent recursion during initialization
static __thread int in_hook = 0;

// Function pointers to real malloc/free
static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;

// Simple temporary allocator for bootstrapping
#define TEMP_BUFFER_SIZE 65536
static char temp_buffer[TEMP_BUFFER_SIZE];
static size_t temp_offset = 0;

static void* temp_malloc(size_t size) {
    if (temp_offset + size >= TEMP_BUFFER_SIZE) {
        return NULL;
    }
    void* ptr = temp_buffer + temp_offset;
    temp_offset += (size + 15) & ~15; // 16-byte alignment
    return ptr;
}

// Initialize function pointers
static void init_hooks(void) {
    if (real_malloc != NULL) return;
    
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_free = dlsym(RTLD_NEXT, "free");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    
    if (!real_malloc || !real_free || !real_calloc || !real_realloc) {
        fprintf(stderr, "Failed to find real allocation functions\n");
        _exit(1);
    }
}

// Constructor - runs before main()
__attribute__((constructor))
static void malloc_hook_init(void) {
    init_hooks();
    fprintf(stderr, "[HOOK] LD_PRELOAD malloc interceptor initialized\n");
    fprintf(stderr, "[HOOK] Intercepting: malloc, free, calloc, realloc\n");
}

// Destructor - runs after main()
__attribute__((destructor))
static void malloc_hook_fini(void) {
    fprintf(stderr, "\n[HOOK] Final Statistics:\n");
    fprintf(stderr, "[HOOK]   Total allocations: %zu\n", atomic_load(&total_allocations));
    fprintf(stderr, "[HOOK]   Total frees: %zu\n", atomic_load(&total_frees));
    fprintf(stderr, "[HOOK]   Bytes allocated: %zu\n", atomic_load(&bytes_allocated));
    fprintf(stderr, "[HOOK]   Bytes freed: %zu\n", atomic_load(&bytes_freed));
    fprintf(stderr, "[HOOK]   Peak usage: %zu bytes\n", atomic_load(&peak_usage));
    fprintf(stderr, "[HOOK]   Current leaks: %zu bytes\n", atomic_load(&current_usage));
}

void* malloc(size_t size) {
    // Prevent recursion during initialization
    if (real_malloc == NULL) {
        init_hooks();
        if (real_malloc == NULL) {
            return temp_malloc(size);
        }
    }
    
    if (in_hook) {
        return real_malloc(size);
    }
    
    in_hook = 1;
    void* ptr = real_malloc(size);
    
    if (ptr != NULL) {
        atomic_fetch_add(&total_allocations, 1);
        atomic_fetch_add(&bytes_allocated, size);
        size_t new_usage = atomic_fetch_add(&current_usage, size) + size;
        
        // Update peak if needed
        size_t current_peak = atomic_load(&peak_usage);
        while (new_usage > current_peak) {
            if (atomic_compare_exchange_weak(&peak_usage, &current_peak, new_usage)) {
                break;
            }
        }
        
        pthread_mutex_lock(&log_mutex);
        fprintf(stderr, "[MALLOC] %p = malloc(%zu) [total: %zu bytes]\n", 
                ptr, size, new_usage);
        pthread_mutex_unlock(&log_mutex);
    }
    
    in_hook = 0;
    return ptr;
}

void free(void* ptr) {
    if (ptr == NULL) return;
    
    // Don't try to free from temp buffer
    if (ptr >= (void*)temp_buffer && ptr < (void*)(temp_buffer + TEMP_BUFFER_SIZE)) {
        return;
    }
    
    if (real_free == NULL) {
        init_hooks();
        if (real_free == NULL) return;
    }
    
    if (in_hook) {
        real_free(ptr);
        return;
    }
    
    in_hook = 1;
    
    atomic_fetch_add(&total_frees, 1);
    
    pthread_mutex_lock(&log_mutex);
    fprintf(stderr, "[FREE] free(%p)\n", ptr);
    pthread_mutex_unlock(&log_mutex);
    
    real_free(ptr);
    in_hook = 0;
}

void* calloc(size_t nmemb, size_t size) {
    if (real_calloc == NULL) {
        init_hooks();
        if (real_calloc == NULL) {
            void* ptr = temp_malloc(nmemb * size);
            if (ptr) memset(ptr, 0, nmemb * size);
            return ptr;
        }
    }
    
    if (in_hook) {
        return real_calloc(nmemb, size);
    }
    
    in_hook = 1;
    void* ptr = real_calloc(nmemb, size);
    
    if (ptr != NULL) {
        size_t total_size = nmemb * size;
        atomic_fetch_add(&total_allocations, 1);
        atomic_fetch_add(&bytes_allocated, total_size);
        size_t new_usage = atomic_fetch_add(&current_usage, total_size) + total_size;
        
        pthread_mutex_lock(&log_mutex);
        fprintf(stderr, "[CALLOC] %p = calloc(%zu, %zu) [total: %zu bytes]\n", 
                ptr, nmemb, size, new_usage);
        pthread_mutex_unlock(&log_mutex);
    }
    
    in_hook = 0;
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (real_realloc == NULL) {
        init_hooks();
        if (real_realloc == NULL) {
            return NULL;
        }
    }
    
    if (in_hook) {
        return real_realloc(ptr, size);
    }
    
    in_hook = 1;
    void* new_ptr = real_realloc(ptr, size);
    
    if (new_ptr != NULL) {
        pthread_mutex_lock(&log_mutex);
        fprintf(stderr, "[REALLOC] %p = realloc(%p, %zu)\n", new_ptr, ptr, size);
        pthread_mutex_unlock(&log_mutex);
    }
    
    in_hook = 0;
    return new_ptr;
}
