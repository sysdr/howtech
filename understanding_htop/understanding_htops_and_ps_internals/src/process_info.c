#include "process_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>

static long page_size = 0;
static long clock_ticks = 0;

void init_system_info(void) {
    page_size = sysconf(_SC_PAGESIZE);
    clock_ticks = sysconf(_SC_CLK_TCK);
}

int parse_proc_stat(pid_t pid, struct process_info *info) {
    char path[256];
    char line[4096];
    FILE *f;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    
    /* Parse /proc/[pid]/stat format:
     * pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt 
     * utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss ...
     */
    
    /* Find command name in parentheses */
    char *start = strchr(line, '(');
    char *end = strrchr(line, ')');
    if (!start || !end) {
        return -1;
    }
    
    /* Extract command */
    size_t cmd_len = end - start - 1;
    if (cmd_len >= sizeof(info->comm)) {
        cmd_len = sizeof(info->comm) - 1;
    }
    memcpy(info->comm, start + 1, cmd_len);
    info->comm[cmd_len] = '\0';
    
    /* Parse fields after command */
    unsigned long utime, stime, vsize;
    long rss;
    int num_threads;
    
    int ret = sscanf(end + 2, 
        "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
        "%lu %lu %*d %*d %*d %*d %d %*d %*u %lu %ld",
        &info->state, 
        &info->ppid,
        &utime,
        &stime,
        &num_threads,
        &vsize,
        &rss);
    
    if (ret < 7) {
        return -1;
    }
    
    info->pid = pid;
    info->utime = utime;
    info->stime = stime;
    info->num_threads = num_threads;
    info->vsize = vsize;
    info->rss = rss * page_size; /* Convert pages to bytes */
    
    return 0;
}

int parse_proc_status(pid_t pid, struct process_info *info) {
    char path[256];
    char line[256];
    FILE *f;
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), f)) {
        unsigned long kb;
        if (sscanf(line, "VmRSS: %lu kB", &kb) == 1) {
            info->rss = kb * 1024;
        } else if (sscanf(line, "VmSize: %lu kB", &kb) == 1) {
            info->vsize = kb * 1024;
        } else if (sscanf(line, "RssAnon: %lu kB", &kb) == 1) {
            info->rss_anon = kb * 1024;
        } else if (sscanf(line, "RssFile: %lu kB", &kb) == 1) {
            info->rss_file = kb * 1024;
        } else if (sscanf(line, "RssShmem: %lu kB", &kb) == 1) {
            info->rss_shmem = kb * 1024;
        }
    }
    
    fclose(f);
    return 0;
}

int get_fd_count(pid_t pid) {
    char path[256];
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    
    snprintf(path, sizeof(path), "/proc/%d/fd", pid);
    dir = opendir(path);
    if (!dir) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

void calculate_cpu_usage(struct process_info *prev, struct process_info *curr, 
                         unsigned long elapsed_ticks, int num_cpus,
                         double *user_pct, double *sys_pct) {
    unsigned long delta_utime = curr->utime - prev->utime;
    unsigned long delta_stime = curr->stime - prev->stime;
    
    /* CPU% = (delta_time / elapsed_time) * 100 * num_cpus */
    *user_pct = (100.0 * delta_utime * num_cpus) / elapsed_ticks;
    *sys_pct = (100.0 * delta_stime * num_cpus) / elapsed_ticks;
}

int get_process_list(pid_t **pids, int *count) {
    DIR *proc;
    struct dirent *entry;
    pid_t *list = NULL;
    int capacity = 128;
    int n = 0;
    
    list = malloc(capacity * sizeof(pid_t));
    if (!list) {
        return -1;
    }
    
    proc = opendir("/proc");
    if (!proc) {
        free(list);
        return -1;
    }
    
    while ((entry = readdir(proc)) != NULL) {
        if (!isdigit(entry->d_name[0])) {
            continue;
        }
        
        pid_t pid = atoi(entry->d_name);
        if (pid == 0) {
            continue;
        }
        
        if (n >= capacity) {
            capacity *= 2;
            pid_t *new_list = realloc(list, capacity * sizeof(pid_t));
            if (!new_list) {
                free(list);
                closedir(proc);
                return -1;
            }
            list = new_list;
        }
        
        list[n++] = pid;
    }
    
    closedir(proc);
    
    *pids = list;
    *count = n;
    return 0;
}

long get_clock_ticks(void) {
    return clock_ticks;
}

int get_num_cpus(void) {
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}
