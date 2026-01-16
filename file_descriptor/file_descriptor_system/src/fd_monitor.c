#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH 4096

typedef struct {
    int pid;
    char comm[256];
    int fd_count;
    int regular_files;
    int sockets;
    int pipes;
    int deleted_files;
} process_info_t;

// Count FDs for a specific process
int count_process_fds(int pid, process_info_t *info) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "/proc/%d/fd", pid);
    
    DIR *dir = opendir(path);
    if (!dir) return -1;
    
    info->pid = pid;
    info->fd_count = 0;
    info->regular_files = 0;
    info->sockets = 0;
    info->pipes = 0;
    info->deleted_files = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char link_path[MAX_PATH];
        char target[MAX_PATH];
        int ret = snprintf(link_path, sizeof(link_path), "%s/%s", path, entry->d_name);
        if (ret < 0 || ret >= (int)sizeof(link_path)) continue; // Skip if truncated
        
        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        if (len > 0) {
            target[len] = '\0';
            info->fd_count++;
            
            if (strstr(target, "socket:")) {
                info->sockets++;
            } else if (strstr(target, "pipe:")) {
                info->pipes++;
            } else if (strstr(target, "(deleted)")) {
                info->deleted_files++;
                info->regular_files++;
            } else {
                info->regular_files++;
            }
        }
    }
    closedir(dir);
    
    // Get process name
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE *f = fopen(path, "r");
    if (f) {
        if (fgets(info->comm, sizeof(info->comm), f)) {
            // Remove newline
            info->comm[strcspn(info->comm, "\n")] = 0;
        }
        fclose(f);
    }
    
    return 0;
}

// Monitor specific PID
void monitor_pid(int target_pid) {
    printf("\033[2J\033[H"); // Clear screen
    printf("FD Monitor - Tracking PID %d\n", target_pid);
    printf("Press Ctrl+C to stop\n\n");
    
    process_info_t info;
    int iteration = 0;
    
    while (1) {
        if (count_process_fds(target_pid, &info) < 0) {
            printf("Process %d no longer exists\n", target_pid);
            break;
        }
        
        time_t now = time(NULL);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&now));
        
        printf("\033[2J\033[H"); // Clear screen
        printf("╔══════════════════════════════════════════════════════════════╗\n");
        printf("║  FD Monitor - Real-time File Descriptor Tracking            ║\n");
        printf("╠══════════════════════════════════════════════════════════════╣\n");
        printf("║ Time: %-50s ║\n", time_str);
        printf("║ PID:  %-50d ║\n", info.pid);
        printf("║ Name: %-50s ║\n", info.comm);
        printf("╠══════════════════════════════════════════════════════════════╣\n");
        printf("║ %-30s %27d ║\n", "Total FDs:", info.fd_count);
        printf("║ %-30s %27d ║\n", "Regular Files:", info.regular_files);
        printf("║ %-30s %27d ║\n", "Sockets:", info.sockets);
        printf("║ %-30s %27d ║\n", "Pipes:", info.pipes);
        printf("║ %-30s %27d ║\n", "Deleted Files (leaking):", info.deleted_files);
        printf("╠══════════════════════════════════════════════════════════════╣\n");
        printf("║ Iterations: %-47d ║\n", ++iteration);
        printf("╚══════════════════════════════════════════════════════════════╝\n");
        
        if (info.deleted_files > 0) {
            printf("\n⚠️  WARNING: %d deleted files still open (disk space not freed)\n", 
                   info.deleted_files);
        }
        
        if (info.fd_count > 1000) {
            printf("\n⚠️  WARNING: High FD count (%d) - possible leak\n", info.fd_count);
        }
        
        printf("\nCommands:\n");
        printf("  lsof -p %d          # Show all FDs with paths\n", target_pid);
        printf("  ls /proc/%d/fd      # Direct /proc inspection\n", target_pid);
        printf("  cat /proc/%d/limits # Check FD limits\n", target_pid);
        
        sleep(1);
    }
}

// System-wide FD summary
void system_summary() {
    printf("Scanning all processes...\n");
    
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir /proc");
        return;
    }
    
    process_info_t top_processes[10];
    int count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;
        
        int pid = atoi(entry->d_name);
        process_info_t info;
        
        if (count_process_fds(pid, &info) == 0 && info.fd_count > 10) {
            // Insert into top 10
            if (count < 10) {
                top_processes[count++] = info;
            } else {
                // Find minimum
                int min_idx = 0;
                for (int i = 1; i < 10; i++) {
                    if (top_processes[i].fd_count < top_processes[min_idx].fd_count) {
                        min_idx = i;
                    }
                }
                if (info.fd_count > top_processes[min_idx].fd_count) {
                    top_processes[min_idx] = info;
                }
            }
        }
    }
    closedir(proc);
    
    // Sort descending
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (top_processes[j].fd_count > top_processes[i].fd_count) {
                process_info_t tmp = top_processes[i];
                top_processes[i] = top_processes[j];
                top_processes[j] = tmp;
            }
        }
    }
    
    printf("\nTop %d processes by FD count:\n", count);
    printf("%-8s %-20s %-10s %-10s %-10s\n", "PID", "Name", "Total FDs", "Files", "Sockets");
    printf("------------------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%-8d %-20s %-10d %-10d %-10d", 
               top_processes[i].pid,
               top_processes[i].comm,
               top_processes[i].fd_count,
               top_processes[i].regular_files,
               top_processes[i].sockets);
        
        if (top_processes[i].deleted_files > 0) {
            printf(" [%d DELETED]", top_processes[i].deleted_files);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mode> [pid]\n", argv[0]);
        printf("Modes:\n");
        printf("  watch <pid>  - Monitor specific process\n");
        printf("  summary      - System-wide FD summary\n");
        return 1;
    }
    
    if (strcmp(argv[1], "watch") == 0 && argc > 2) {
        int pid = atoi(argv[2]);
        monitor_pid(pid);
    } else if (strcmp(argv[1], "summary") == 0) {
        system_summary();
    }
    
    return 0;
}
