#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_IRQS 256
#define MAX_CPUS 16

typedef struct {
    int irq_num;
    char name[64];
    unsigned long counts[MAX_CPUS];
    unsigned long prev_counts[MAX_CPUS];
} irq_info_t;

static irq_info_t irqs[MAX_IRQS];
static int num_irqs = 0;
static int num_cpus = 0;

void parse_interrupts(void) {
    FILE *fp = fopen("/proc/interrupts", "r");
    if (!fp) {
        perror("fopen /proc/interrupts");
        return;
    }
    
    char line[1024];
    int is_header = 1;
    num_irqs = 0;
    
    while (fgets(line, sizeof(line), fp) && num_irqs < MAX_IRQS) {
        if (is_header) {
            // Count CPUs from header
            num_cpus = 0;
            char *token = strtok(line, " ");
            while (token != NULL && num_cpus < MAX_CPUS) {
                if (strncmp(token, "CPU", 3) == 0) num_cpus++;
                token = strtok(NULL, " ");
            }
            is_header = 0;
            continue;
        }
        
        // Parse IRQ line
        int irq_num;
        if (sscanf(line, "%d:", &irq_num) == 1) {
            irqs[num_irqs].irq_num = irq_num;
            
            // Save previous counts
            for (int i = 0; i < num_cpus; i++) {
                irqs[num_irqs].prev_counts[i] = irqs[num_irqs].counts[i];
            }
            
            // Parse counts for each CPU
            char *ptr = strchr(line, ':');
            if (ptr) {
                ptr++;
                for (int i = 0; i < num_cpus && i < MAX_CPUS; i++) {
                    irqs[num_irqs].counts[i] = strtoul(ptr, &ptr, 10);
                }
                
                // Get IRQ name
                while (*ptr == ' ') ptr++;
                char *name_start = ptr;
                char *name_end = strchr(ptr, '\n');
                if (name_end) {
                    int len = name_end - name_start;
                    if (len > 63) len = 63;
                    strncpy(irqs[num_irqs].name, name_start, len);
                    irqs[num_irqs].name[len] = '\0';
                }
            }
            
            num_irqs++;
        }
    }
    
    fclose(fp);
}

void print_irq_stats(void) {
    printf("\033[2J\033[H"); // Clear screen
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           ARM Interrupt Monitor (/proc/interrupts)           ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    printf("║ CPUs: %d                                                      ║\n", num_cpus);
    printf("╠════╦═══════╦");
    for (int i = 0; i < num_cpus && i < 8; i++) printf("═════════╦");
    printf("\n");
    
    printf("║IRQ ║ Total ║");
    for (int i = 0; i < num_cpus && i < 8; i++) printf(" CPU%-2d   ║", i);
    printf("\n");
    
    printf("╠════╬═══════╬");
    for (int i = 0; i < num_cpus && i < 8; i++) printf("═════════╬");
    printf("\n");
    
    // Show top IRQs by delta
    for (int i = 0; i < num_irqs && i < 20; i++) {
        unsigned long total = 0;
        unsigned long delta = 0;
        
        for (int c = 0; c < num_cpus; c++) {
            total += irqs[i].counts[c];
            delta += (irqs[i].counts[c] - irqs[i].prev_counts[c]);
        }
        
        if (delta > 0) {
            printf("║%3d ║%7lu║", irqs[i].irq_num, delta);
            for (int c = 0; c < num_cpus && c < 8; c++) {
                unsigned long cpu_delta = irqs[i].counts[c] - irqs[i].prev_counts[c];
                if (cpu_delta > 0) {
                    printf(" \033[1;32m%7lu\033[0m║", cpu_delta);
                } else {
                    printf(" %7lu║", cpu_delta);
                }
            }
            printf(" %s\n", irqs[i].name);
        }
    }
    
    printf("╚════╩═══════╩");
    for (int i = 0; i < num_cpus && i < 8; i++) printf("═════════╩");
    printf("\n");
    
    printf("\nPress Ctrl+C to exit...\n");
}

int main(void) {
    printf("Starting ARM Interrupt Monitor...\n");
    printf("Reading /proc/interrupts every second\n\n");
    sleep(1);
    
    parse_interrupts(); // Initial parse
    
    while (1) {
        sleep(1);
        parse_interrupts();
        print_irq_stats();
    }
    
    return 0;
}
