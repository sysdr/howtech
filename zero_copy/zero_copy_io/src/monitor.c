#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

void print_header(void) {
    printf("\033[2J\033[H");
    printf("\033[1;34m╔════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;34m║        Zero-Copy I/O Performance Monitor                      ║\033[0m\n");
    printf("\033[1;34m╚════════════════════════════════════════════════════════════════╝\033[0m\n\n");
}

void read_proc_io(pid_t pid, long long *syscr, long long *syscw, 
                  long long *rchar, long long *wchar) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "syscr: %lld", syscr) == 1) continue;
        if (sscanf(line, "syscw: %lld", syscw) == 1) continue;
        if (sscanf(line, "rchar: %lld", rchar) == 1) continue;
        if (sscanf(line, "wchar: %lld", wchar) == 1) continue;
    }
    
    fclose(fp);
}

void read_proc_stat(pid_t pid, unsigned long *utime, unsigned long *stime,
                    long *minflt, long *majflt) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return;
    
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, cminflt, cmajflt;
    
    if (fscanf(fp, "%*d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu",
               comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
               &flags, minflt, &cminflt, majflt, &cmajflt, utime, stime) < 14) {
        fclose(fp);
        return;
    }
    
    fclose(fp);
}

void monitor_process(const char *name, int port) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -f '%s.*data/testfile'", name);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    
    pid_t pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        pclose(fp);
        return;
    }
    pclose(fp);
    
    long long syscr, syscw, rchar, wchar;
    unsigned long utime, stime;
    long minflt, majflt;
    
    read_proc_io(pid, &syscr, &syscw, &rchar, &wchar);
    read_proc_stat(pid, &utime, &stime, &minflt, &majflt);
    
    printf("\033[1;33m%s Server (Port %d)\033[0m\n", name, port);
    printf("  PID: %d\n", pid);
    printf("  \033[0;36mSyscall Stats:\033[0m\n");
    printf("    Read syscalls:  %lld\n", syscr);
    printf("    Write syscalls: %lld\n", syscw);
    printf("  \033[0;36mI/O Stats:\033[0m\n");
    printf("    Bytes read:     %lld\n", rchar);
    printf("    Bytes written:  %lld\n", wchar);
    printf("  \033[0;36mCPU Time:\033[0m\n");
    printf("    User time:      %lu jiffies\n", utime);
    printf("    System time:    %lu jiffies\n", stime);
    printf("  \033[0;36mPage Faults:\033[0m\n");
    printf("    Minor faults:   %ld\n", minflt);
    printf("    Major faults:   %ld\n", majflt);
    printf("\n");
}

int main(void) {
    while (1) {
        print_header();
        
        monitor_process("traditional_server", 8080);
        monitor_process("sendfile_server", 8081);
        monitor_process("splice_server", 8082);
        
        printf("\033[0;90mPress Ctrl+C to stop monitoring...\033[0m\n");
        sleep(2);
    }
    
    return 0;
}
