#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>

#define PROC_PATH "/proc"
#define MAX_LINE 256

typedef struct {
    int pid;
    char name[64];
    int priority;
    char policy[16];
    unsigned long voluntary_switches;
    unsigned long involuntary_switches;
} task_info_t;

static void get_task_info(int pid, task_info_t *info) {
    char path[256];
    FILE *fp;
    char line[MAX_LINE];
    
    info->pid = pid;
    info->priority = -1;
    strcpy(info->policy, "UNKNOWN");
    info->voluntary_switches = 0;
    info->involuntary_switches = 0;
    
    /* Get name from status */
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line + 5, "%s", info->name);
            }
        }
        fclose(fp);
    }
    
    /* Get scheduling info */
    snprintf(path, sizeof(path), "/proc/%d/sched", pid);
    fp = fopen(path, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "prio", 4) == 0) {
                sscanf(line, "%*s : %d", &info->priority);
            } else if (strstr(line, "nr_voluntary_switches")) {
                sscanf(line, "%*s : %lu", &info->voluntary_switches);
            } else if (strstr(line, "nr_involuntary_switches")) {
                sscanf(line, "%*s : %lu", &info->involuntary_switches);
            } else if (strncmp(line, "policy", 6) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    int policy_num;
                    sscanf(colon + 1, "%d", &policy_num);
                    switch(policy_num) {
                        case 0: strcpy(info->policy, "NORMAL"); break;
                        case 1: strcpy(info->policy, "FIFO"); break;
                        case 2: strcpy(info->policy, "RR"); break;
                        case 3: strcpy(info->policy, "BATCH"); break;
                        case 5: strcpy(info->policy, "IDLE"); break;
                        case 6: strcpy(info->policy, "DEADLINE"); break;
                        default: sprintf(info->policy, "UNK(%d)", policy_num);
                    }
                }
            }
        }
        fclose(fp);
    }
}

static void print_header(void) {
    printf("\033[2J\033[H"); // Clear screen and home cursor
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Real-Time Task Monitor - Priority Inversion Demo            ║\n");
    printf("╠════════╦══════════════════╦══════════╦═════════╦═══════════╦═══════════╣\n");
    printf("║  PID   ║      NAME        ║ PRIORITY ║ POLICY  ║  VOL_SW   ║ INVOL_SW  ║\n");
    printf("╠════════╬══════════════════╬══════════╬═════════╬═══════════╬═══════════╣\n");
}

static void print_task(task_info_t *info) {
    const char *color = "";
    
    if (strcmp(info->policy, "FIFO") == 0 || strcmp(info->policy, "RR") == 0) {
        if (info->priority >= 80) {
            color = "\033[1;31m"; // Bright red for high priority
        } else if (info->priority >= 40) {
            color = "\033[1;33m"; // Bright yellow for medium priority
        } else {
            color = "\033[1;36m"; // Bright cyan for low priority
        }
    }
    
    printf("║ %s%-6d\033[0m ║ %-16s ║ %8d ║ %-7s ║ %9lu ║ %9lu ║\n",
           color, info->pid, info->name, info->priority, info->policy,
           info->voluntary_switches, info->involuntary_switches);
}

static void print_footer(void) {
    printf("╚════════╩══════════════════╩══════════╩═════════╩═══════════╩═══════════╝\n");
    printf("\nColor coding: \033[1;31mHigh Priority (80+)\033[0m  ");
    printf("\033[1;33mMedium Priority (40-79)\033[0m  ");
    printf("\033[1;36mLow Priority (<40)\033[0m\n");
    printf("VOL_SW = Voluntary context switches | INVOL_SW = Involuntary switches\n");
    printf("Press Ctrl+C to exit\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program_name>\n", argv[0]);
        return 1;
    }
    
    const char *target_name = argv[1];
    
    while (1) {
        DIR *dir = opendir(PROC_PATH);
        if (!dir) {
            perror("opendir");
            return 1;
        }
        
        task_info_t tasks[100];
        int task_count = 0;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && task_count < 100) {
            if (!isdigit(entry->d_name[0])) continue;
            
            int pid = atoi(entry->d_name);
            task_info_t info;
            get_task_info(pid, &info);
            
            if (strstr(info.name, target_name) != NULL) {
                tasks[task_count++] = info;
            }
        }
        closedir(dir);
        
        print_header();
        for (int i = 0; i < task_count; i++) {
            print_task(&tasks[i]);
        }
        print_footer();
        
        sleep(1);
    }
    
    return 0;
}
