#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#define LOG_FILE "output/vruntime_log.txt"
#define OUTPUT_DIR "output"
#define SAMPLE_INTERVAL_MS 100
#define DURATION_SEC 12

typedef struct {
    pid_t pid;
    char name[64];
    unsigned long long vruntime;
    unsigned long long exec_time;
    time_t timestamp;
} sample_t;

int read_task_vruntime(pid_t pid, sample_t *sample) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/sched", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    sample->pid = pid;
    sample->timestamp = time(NULL);
    
    char line[512];
    int found_name = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (!found_name) {
            sscanf(line, "%63s", sample->name);
            found_name = 1;
            continue;
        }
        
        if (strstr(line, "se.vruntime")) {
            sscanf(line, " se.vruntime : %llu", &sample->vruntime);
        } else if (strstr(line, "se.sum_exec_runtime")) {
            sscanf(line, " se.sum_exec_runtime : %llu", &sample->exec_time);
        }
    }
    
    fclose(f);
    return 0;
}

int find_demo_tasks(pid_t pids[], int max_pids) {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return 0;
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(proc_dir)) && count < max_pids) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        
        FILE *f = fopen(path, "r");
        if (f) {
            char comm[64];
            if (fgets(comm, sizeof(comm), f)) {
                if (strstr(comm, "cfs_demo")) {
                    pids[count++] = pid;
                }
            }
            fclose(f);
        }
    }
    
    closedir(proc_dir);
    return count;
}

int main(void) {
    // Ensure output directory exists
    struct stat st = {0};
    if (stat(OUTPUT_DIR, &st) == -1) {
        char mkdir_cmd[256];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", OUTPUT_DIR);
        int ret = system(mkdir_cmd);
        (void)ret;  // Suppress unused warning
    }
    
    FILE *log = fopen(LOG_FILE, "w");
    if (!log) {
        perror("fopen");
        return 1;
    }
    
    fprintf(log, "# CFS Vruntime Analysis Log\n");
    fprintf(log, "# Timestamp, PID, Task, Vruntime, ExecTime(ns)\n\n");
    
    printf("Logging vruntime progression to %s...\n", LOG_FILE);
    printf("Duration: %d seconds\n", DURATION_SEC);
    
    time_t start_time = time(NULL);
    pid_t tracked_pids[128];
    int num_tracked = 0;
    
    while (time(NULL) - start_time < DURATION_SEC) {
        // Refresh list of demo tasks
        num_tracked = find_demo_tasks(tracked_pids, 128);
        
        for (int i = 0; i < num_tracked; i++) {
            sample_t sample;
            if (read_task_vruntime(tracked_pids[i], &sample) == 0) {
                fprintf(log, "%ld, %d, %s, %llu, %llu\n",
                        sample.timestamp, sample.pid, sample.name,
                        sample.vruntime, sample.exec_time);
            }
        }
        
        fflush(log);
        usleep(SAMPLE_INTERVAL_MS * 1000);
    }
    
    fclose(log);
    printf("Logging complete. Analyze with: column -t -s',' %s\n", LOG_FILE);
    
    return 0;
}
