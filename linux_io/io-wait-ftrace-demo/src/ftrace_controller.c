#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

// Modern kernels use /sys/kernel/tracing, fallback to debugfs
static const char *tracefs_paths[] = {
    "/sys/kernel/tracing",
    "/sys/kernel/debug/tracing",
    NULL
};

static const char *tracefs_root = NULL;
static int tracing_was_enabled = 0;

static void find_tracefs(void) {
    for (int i = 0; tracefs_paths[i] != NULL; i++) {
        if (access(tracefs_paths[i], F_OK) == 0) {
            tracefs_root = tracefs_paths[i];
            printf("Found tracefs at: %s\n", tracefs_root);
            return;
        }
    }
    fprintf(stderr, "ERROR: tracefs not found. Ensure kernel has CONFIG_TRACING=y\n");
    exit(1);
}

static void write_to_tracefs(const char *file, const char *value) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", tracefs_root, file);
    
    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return;
    }
    
    ssize_t written = write(fd, value, strlen(value));
    if (written < 0) {
        fprintf(stderr, "Failed to write to %s: %s\n", path, strerror(errno));
    }
    
    close(fd);
}

static void read_from_tracefs(const char *file) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", tracefs_root, file);
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return;
    }
    
    char buffer[32];
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        tracing_was_enabled = (buffer[0] == '1');
    }
    close(fd);
}

static void cleanup_tracing(int sig) {
    (void)sig;  // Suppress unused parameter warning
    printf("\n\nCleaning up ftrace configuration...\n");
    write_to_tracefs("tracing_on", "0");
    write_to_tracefs("events/block/enable", "0");
    write_to_tracefs("trace", "");
    
    if (!tracing_was_enabled) {
        printf("Restored tracing to disabled state\n");
    }
    
    exit(0);
}

void setup_ftrace_block_tracing(void) {
    printf("\n=== Setting up Ftrace Block Tracing ===\n");
    
    find_tracefs();
    
    // Check if tracing was already enabled
    read_from_tracefs("tracing_on");
    
    // Install cleanup handler
    signal(SIGINT, cleanup_tracing);
    signal(SIGTERM, cleanup_tracing);
    
    // Clear existing trace
    printf("Clearing existing trace buffer...\n");
    write_to_tracefs("trace", "");
    
    // Set larger buffer to avoid overruns
    printf("Setting trace buffer size to 16MB per CPU...\n");
    write_to_tracefs("buffer_size_kb", "16384");
    
    // Enable block layer events
    printf("Enabling block layer tracepoints...\n");
    write_to_tracefs("events/block/block_rq_issue/enable", "1");
    write_to_tracefs("events/block/block_rq_complete/enable", "1");
    write_to_tracefs("events/block/block_bio_queue/enable", "1");
    
    // Start tracing
    printf("Starting trace...\n");
    write_to_tracefs("tracing_on", "1");
    
    printf("Ftrace block tracing is now active!\n");
    printf("Press Ctrl+C to stop tracing and analyze results\n\n");
}

int main(void) {
    if (geteuid() != 0) {
        fprintf(stderr, "ERROR: This program requires root privileges\n");
        return 1;
    }
    
    setup_ftrace_block_tracing();
    
    // Keep running until interrupted
    printf("Tracing active. Waiting for workload...\n");
    pause();
    
    return 0;
}
