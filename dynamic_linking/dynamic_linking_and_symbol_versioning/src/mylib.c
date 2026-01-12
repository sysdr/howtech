#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Version 1.0 implementations */
int api_init_v1(const char *config) {
    printf("[v1.0] Initializing with config: %s\n", config);
    return 0;
}

int api_process_v1(const char *data) {
    printf("[v1.0] Processing: %s\n", data);
    return strlen(data);
}

void api_cleanup_v1(void) {
    printf("[v1.0] Cleanup completed\n");
}

/* Version 2.0 implementations - enhanced versions */
typedef struct {
    int status;
    int bytes_processed;
    int errors;
} api_stats_t;

static api_stats_t global_stats = {0, 0, 0};

__asm__(".symver api_init_v1, api_init@@MYLIB_1.0");
__asm__(".symver api_process_v1, api_process@@MYLIB_1.0");
__asm__(".symver api_cleanup_v1, api_cleanup@@MYLIB_1.0");

int api_init_v2_impl(const char *config, int flags) {
    printf("[v2.0] Initializing with config: %s, flags: 0x%x\n", config, flags);
    global_stats.status = 1;
    return 0;
}

int api_process_extended_v2_impl(const char *data, size_t len) {
    printf("[v2.0] Processing %zu bytes: %.*s\n", len, (int)len, data);
    global_stats.bytes_processed += len;
    return len;
}

const api_stats_t* api_get_stats_v2_impl(void) {
    return &global_stats;
}

/* Version 3.0 implementations */
int api_process_batch_v3_impl(const char **data_array, int count) {
    printf("[v3.0] Batch processing %d items\n", count);
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += strlen(data_array[i]);
    }
    return total;
}

void api_reset_v3_impl(void) {
    printf("[v3.0] Resetting all state\n");
    memset(&global_stats, 0, sizeof(global_stats));
}

/* Symbol versioning directives for v2.0 and v3.0 */
__asm__(".symver api_init_v2_impl, api_init_v2@@MYLIB_2.0");
__asm__(".symver api_process_extended_v2_impl, api_process_extended@@MYLIB_2.0");
__asm__(".symver api_get_stats_v2_impl, api_get_stats@@MYLIB_2.0");

__asm__(".symver api_process_batch_v3_impl, api_process_batch@@MYLIB_3.0");
__asm__(".symver api_reset_v3_impl, api_reset@@MYLIB_3.0");
