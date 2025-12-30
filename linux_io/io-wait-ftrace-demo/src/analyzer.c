#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SECTORS 1000000
#define MAX_LINE 512

typedef struct {
    double timestamp;
    unsigned long sector;
    int size;
} bio_event_t;

static bio_event_t issued[MAX_SECTORS];
static int num_issued = 0;
static double latencies[MAX_SECTORS];
static int num_latencies = 0;

static int compare_doubles(const void *a, const void *b) {
    double diff = *(double*)a - *(double*)b;
    return (diff > 0) - (diff < 0);
}

static void calculate_percentile_latencies(void) {
    if (num_latencies == 0) {
        printf("No latency samples collected\n");
        return;
    }
    
    qsort(latencies, num_latencies, sizeof(double), compare_doubles);
    
    printf("\n=== Latency Distribution ===\n");
    printf("Total I/O operations: %d\n", num_latencies);
    printf("Min latency:  %.3f ms\n", latencies[0]);
    printf("P50 latency:  %.3f ms\n", latencies[num_latencies / 2]);
    printf("P95 latency:  %.3f ms\n", latencies[num_latencies * 95 / 100]);
    printf("P99 latency:  %.3f ms\n", latencies[num_latencies * 99 / 100]);
    printf("Max latency:  %.3f ms\n", latencies[num_latencies - 1]);
    
    // Calculate average
    double sum = 0.0;
    for (int i = 0; i < num_latencies; i++) {
        sum += latencies[i];
    }
    printf("Avg latency:  %.3f ms\n", sum / num_latencies);
}

void analyze_trace_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open trace file: %s\n", filename);
        return;
    }
    
    char line[MAX_LINE];
    int issue_count = 0, complete_count = 0;
    
    printf("Analyzing trace file: %s\n", filename);
    
    while (fgets(line, sizeof(line), fp)) {
        // Skip header lines
        if (line[0] == '#' || strstr(line, "TASK-PID")) {
            continue;
        }
        
        double timestamp;
        unsigned long sector;
        int size;
        
        // Parse block_rq_issue events
        if (strstr(line, "block_rq_issue")) {
            if (sscanf(line, "%*s %*d [%*d] %lf: %*s %*s %lu %*s %d", 
                       &timestamp, &sector, &size) == 3) {
                if (num_issued < MAX_SECTORS) {
                    issued[num_issued].timestamp = timestamp;
                    issued[num_issued].sector = sector;
                    issued[num_issued].size = size;
                    num_issued++;
                    issue_count++;
                }
            }
        }
        // Parse block_rq_complete events and match with issue
        else if (strstr(line, "block_rq_complete")) {
            if (sscanf(line, "%*s %*d [%*d] %lf: %*s %*s %lu %*s %d", 
                       &timestamp, &sector, &size) == 3) {
                complete_count++;
                
                // Find matching issue event
                for (int i = 0; i < num_issued; i++) {
                    if (issued[i].sector == sector && issued[i].size == size) {
                        double latency_ms = (timestamp - issued[i].timestamp) * 1000.0;
                        if (latency_ms >= 0 && num_latencies < MAX_SECTORS) {
                            latencies[num_latencies++] = latency_ms;
                        }
                        break;
                    }
                }
            }
        }
    }
    
    fclose(fp);
    
    printf("Parsed %d issue events, %d complete events\n", issue_count, complete_count);
    calculate_percentile_latencies();
}

int main(int argc, char *argv[]) {
    const char *trace_file = (argc > 1) ? argv[1] : "traces/trace.out";
    
    printf("I/O Latency Analyzer\n");
    printf("====================\n\n");
    
    analyze_trace_file(trace_file);
    
    return 0;
}
