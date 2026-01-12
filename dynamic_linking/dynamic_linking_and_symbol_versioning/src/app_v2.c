#include <stdio.h>
#include <stddef.h>

/* Declarations for v2.0 API */
extern int api_init_v2(const char *config, int flags);
extern int api_process_extended(const char *data, size_t len);

typedef struct {
    int status;
    int bytes_processed;
    int errors;
} api_stats_t;

extern const api_stats_t* api_get_stats(void);

int main() {
    printf("Application compiled against MYLIB_2.0\n");
    printf("========================================\n\n");
    
    api_init_v2("config_v2.json", 0x01);
    
    const char *data = "Hello from v2 app";
    int result = api_process_extended(data, 17);
    printf("Processed %d bytes\n", result);
    
    const api_stats_t *stats = api_get_stats();
    printf("\nStats: status=%d, bytes=%d, errors=%d\n\n",
           stats->status, stats->bytes_processed, stats->errors);
    
    return 0;
}
