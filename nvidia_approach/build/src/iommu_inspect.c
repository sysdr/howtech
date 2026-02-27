/*
 * iommu_inspect.c — IOMMU group topology inspection
 *
 * In the NVIDIA ASIL B architecture, the IOMMU configuration is the
 * hardware mechanism that enforces Freedom from Interference (FFI).
 * This tool reads /sys/kernel/iommu_groups/ to show which devices
 * are isolated in which groups — the boundary the safety argument
 * points to.
 *
 * Compile: gcc -Wall -Wextra -Werror -O2 -o iommu_inspect iommu_inspect.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define IOMMU_SYSFS "/sys/kernel/iommu_groups"
#define MAX_PATH    1024

static int cmp_dir(const struct dirent **a, const struct dirent **b) {
    return atoi((*a)->d_name) - atoi((*b)->d_name);
}

static void read_iommu_groups(void) {
    struct dirent **namelist;
    int n;
    char path[MAX_PATH];
    char devpath[2048];

    printf("\033[1;34m  IOMMU Group Topology\033[0m  (from %s)\n\n", IOMMU_SYSFS);

    n = scandir(IOMMU_SYSFS, &namelist, NULL, cmp_dir);
    if (n < 0) {
        printf("  \033[2m%s not available (not running on IOMMU-capable platform,\033[0m\n"
               "  \033[2mor no IOMMU kernel support). This is expected in Docker.\033[0m\n\n"
               "  \033[1mOn a real Orin/x86 IOMMU system:\033[0m\n"
               "  Each IOMMU group = isolated DMA domain.\n"
               "  GPU must be in a group isolated from safety DRAM region.\n"
               "  Verify with: ls /sys/kernel/iommu_groups/*/devices/\n", IOMMU_SYSFS);
        return;
    }

    int shown = 0;
    for (int i = 0; i < n; i++) {
        if (namelist[i]->d_name[0] == '.') { free(namelist[i]); continue; }

        snprintf(path, sizeof(path), "%s/%s/devices", IOMMU_SYSFS, namelist[i]->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) { free(namelist[i]); continue; }

        struct dirent **devs;
        int nd = scandir(path, &devs, NULL, alphasort);
        if (nd < 0) { free(namelist[i]); continue; }

        int has_dev = 0;
        for (int j = 0; j < nd; j++) {
            if (devs[j]->d_name[0] != '.') has_dev++;
            free(devs[j]);
        }
        free(devs);

        if (!has_dev) { free(namelist[i]); continue; }

        printf("  Group %-3s │ %d device(s)\n", namelist[i]->d_name, has_dev);

        /* List devices in group */
        struct dirent **devs2;
        int nd2 = scandir(path, &devs2, NULL, alphasort);
        for (int j = 0; j < nd2; j++) {
            if (devs2[j]->d_name[0] == '.') { free(devs2[j]); continue; }
            snprintf(devpath, sizeof(devpath), "%s/%s", path, devs2[j]->d_name);
            printf("           └─ %s\n", devs2[j]->d_name);
            free(devs2[j]);
        }
        if (nd2 >= 0) free(devs2);
        shown++;
        free(namelist[i]);
    }
    free(namelist);

    if (shown > 0) {
        printf("\n  \033[0;32m✓ %d IOMMU group(s) found\033[0m\n", shown);
        printf("  \033[2mDevices in separate groups have hardware-isolated DMA domains.\033[0m\n");
        printf("  \033[2mSafety argument: GPU cannot DMA into safety DRAM region\033[0m\n");
        printf("  \033[2mif they are in separate groups and IOMMU is armed at boot.\033[0m\n");
    }
}

static void inspect_iomem(void) {
    printf("\n\033[1;34m  /proc/iomem — Memory Region Map\033[0m\n\n");
    FILE *f = fopen("/proc/iomem", "r");
    if (!f) { printf("  (not accessible)\n"); return; }
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < 30) {
        /* highlight RAM and System RAM regions */
        if (strstr(line, "System RAM") || strstr(line, "Kernel code") ||
            strstr(line, "PCI Bus") || strstr(line, "MMCONFIG")) {
            printf("  \033[0;33m%s\033[0m", line);
        } else {
            printf("  %s", line);
        }
        count++;
    }
    if (!feof(f)) printf("  ... (truncated — run `cat /proc/iomem` for full map)\n");
    fclose(f);
}

int main(void) {
    printf("\n\033[1;34m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;34m║\033[0m  IOMMU / Memory Isolation Inspector                  \033[1;34m║\033[0m\n");
    printf("\033[1;34m║\033[0m  Freedom from Interference — hardware evidence        \033[1;34m║\033[0m\n");
    printf("\033[1;34m╚══════════════════════════════════════════════════════╝\033[0m\n\n");

    read_iommu_groups();
    inspect_iomem();

    printf("\n\033[1;33m  Key diagnostic commands on a real Orin system:\033[0m\n");
    printf("  ls /sys/kernel/iommu_groups/*/devices/          # group topology\n");
    printf("  dmesg | grep -i iommu                           # SMMU init + faults\n");
    printf("  dmesg | grep -i 'dma firewall'                  # Orin HW firewall log\n");
    printf("  cat /proc/iomem | grep -i 'carve\\|safety'       # safety carveout\n");
    printf("  cat /sys/bus/platform/devices/*/iommu_group     # per-device group\n\n");
    return EXIT_SUCCESS;
}
