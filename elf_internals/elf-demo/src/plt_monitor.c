#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <stdarg.h>

#define COLOR_HEADER  "\033[1;36m"
#define COLOR_CALL    "\033[1;33m"
#define COLOR_TIME    "\033[0;32m"
#define COLOR_RESET   "\033[0m"

static __thread int in_wrapper = 0;

// Track timing for first vs subsequent calls
static struct {
    int count;
    long first_call_us;
    long total_us;
} stats = {0};

static long get_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

// Interpose printf to show PLT/GOT behavior
int printf(const char *format, ...) __attribute__((no_instrument_function));

int printf(const char *format, ...) {
    if (in_wrapper) {
        // Prevent recursion
        static int (*real_printf)(const char *, ...) = NULL;
        if (!real_printf) {
            real_printf = dlsym(RTLD_NEXT, "printf");
        }
        va_list args;
        va_start(args, format);
        int ret = vprintf(format, args);
        va_end(args);
        return ret;
    }
    
    in_wrapper = 1;
    
    long start = get_usec();
    
    // Call real printf
    static int (*real_printf)(const char *, ...) = NULL;
    if (!real_printf) {
        real_printf = dlsym(RTLD_NEXT, "printf");
        fprintf(stderr, "%s[PLT/GOT Monitor]%s First call to printf - Dynamic linker resolving symbol...\n",
                COLOR_CALL, COLOR_RESET);
    }
    
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    
    long elapsed = get_usec() - start;
    
    if (stats.count == 0) {
        stats.first_call_us = elapsed;
        fprintf(stderr, "%s[PLT/GOT Monitor]%s First call took %s%ld us%s (includes symbol resolution)\n",
                COLOR_CALL, COLOR_RESET, COLOR_TIME, elapsed, COLOR_RESET);
    }
    
    stats.count++;
    stats.total_us += elapsed;
    
    in_wrapper = 0;
    return ret;
}

__attribute__((constructor))
static void init() {
    fprintf(stderr, "\n%s", COLOR_HEADER);
    fprintf(stderr, "╔═════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║     PLT/GOT Symbol Resolution Monitor          ║\n");
    fprintf(stderr, "║  Watching lazy binding in action...            ║\n");
    fprintf(stderr, "╚═════════════════════════════════════════════════╝\n");
    fprintf(stderr, "%s\n", COLOR_RESET);
}

__attribute__((destructor))
static void fini() {
    if (stats.count > 1) {
        long avg = stats.total_us / stats.count;
        long subsequent_avg = (stats.total_us - stats.first_call_us) / (stats.count - 1);
        
        fprintf(stderr, "\n%s", COLOR_HEADER);
        fprintf(stderr, "╔═════════════════════════════════════════════════╗\n");
        fprintf(stderr, "║           PLT/GOT Performance Summary           ║\n");
        fprintf(stderr, "╚═════════════════════════════════════════════════╝\n");
        fprintf(stderr, "%s", COLOR_RESET);
        fprintf(stderr, "  Total calls:         %s%d%s\n", COLOR_TIME, stats.count, COLOR_RESET);
        fprintf(stderr, "  First call:          %s%ld us%s (with resolution)\n",
                COLOR_TIME, stats.first_call_us, COLOR_RESET);
        fprintf(stderr, "  Subsequent average:  %s%ld us%s (direct GOT jump)\n",
                COLOR_TIME, subsequent_avg, COLOR_RESET);
        fprintf(stderr, "  Overall average:     %s%ld us%s\n\n",
                COLOR_TIME, avg, COLOR_RESET);
    }
}
