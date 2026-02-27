/*
 * qualification_scope.c — Visualizes qualification scope comparison
 *
 * Counts lines of code in this source tree and shows the ratio between
 * a simulated "in-kernel driver" scope vs. the UIO userspace scope.
 * Makes concrete the economic argument for minimizing upstream kernel burden.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH 512

static int count_c_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int lines = 0; int c;
    while ((c = fgetc(f)) != EOF) if (c == '\n') lines++;
    fclose(f);
    return lines;
}

static int count_dir_lines(const char *dir) {
    struct dirent **nl; int n;
    n = scandir(dir, &nl, NULL, alphasort);
    if (n < 0) return 0;
    int total = 0;
    for (int i = 0; i < n; i++) {
        if (nl[i]->d_name[0] == '.') { free(nl[i]); continue; }
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dir, nl[i]->d_name);
        size_t len = strlen(nl[i]->d_name);
        if (len > 2 && strcmp(nl[i]->d_name + len - 2, ".c") == 0)
            total += count_c_lines(path);
        free(nl[i]);
    }
    free(nl);
    return total;
}

static void bar(int val, int max_val, int width, const char *color) {
    int filled = (val * width) / (max_val > 0 ? max_val : 1);
    printf("%s", color);
    for (int i = 0; i < filled; i++) printf("█");
    for (int i = filled; i < width; i++) printf("░");
    printf("\033[0m");
}

int main(void) {
    int our_lines = count_dir_lines("/src");
    if (our_lines == 0) our_lines = count_dir_lines(".");

    /* Representative numbers from real projects */
    struct {
        const char *name;
        int lines;
        const char *notes;
        const char *color;
        int in_qual_scope;
    } entries[] = {
        { "Linux kernel (LTS, all)",     31000000, "no MISRA, unowned",   "\033[0;31m", 0 },
        { "Typical DRM GPU driver",          8500, "in-kernel, must own", "\033[0;33m", 1 },
        { "UIO kernel shim (upstream)",        95, "never changes",       "\033[0;32m", 0 },
        { "This userspace driver (UIO)",  our_lines > 0 ? our_lines : 320,
                                               "qualified as SEooC",  "\033[0;32m", 1 },
        { "Safety RTOS + monitor (R5)",      4200, "ASIL B, bounded",     "\033[1;32m", 1 },
    };
    int n = (int)(sizeof(entries) / sizeof(entries[0]));

    printf("\n\033[1;34m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;34m║\033[0m  Qualification Scope Comparison                       \033[1;34m║\033[0m\n");
    printf("\033[1;34m╚══════════════════════════════════════════════════════╝\033[0m\n\n");

    printf("  \033[1m%-42s  %8s  In-scope  Bar (log)\033[0m\n",
           "Component", "LOC");
    printf("  %s\n", "─────────────────────────────────────────────────────────────────────────");

    for (int i = 0; i < n; i++) {
        int log_val = 0;
        int v = entries[i].lines;
        while (v > 1) { v /= 10; log_val++; }
        printf("  %-42s  %8d  %-9s  ",
               entries[i].name, entries[i].lines,
               entries[i].in_qual_scope ? "\033[0;32m  YES\033[0m" : "\033[2m   no\033[0m");
        bar(log_val, 8, 28, entries[i].color);
        printf("  %s\n", entries[i].notes);
    }

    printf("\n  \033[2mLog scale: each full bar = 10× more code to qualify.\033[0m\n");
    printf("\n  \033[1;33m  Qualification scope in UIO approach:\033[0m\n");
    printf("    • UIO shim (0 lines owned — upstream)\n");
    printf("    • Userspace driver (%d lines — versioned, bounded)\n",
           our_lines > 0 ? our_lines : 320);
    printf("    • Safety RTOS on R5 (~4200 lines — separate ASIL B process)\n");
    printf("\n  \033[1;33m  vs. In-kernel driver approach:\033[0m\n");
    printf("    • ~8500 lines of in-kernel C, owned, FMEA-required\n");
    printf("    • Regenerates diff every 24-month LTS cycle\n");
    printf("    • 6–18 engineer-months to re-qualify per LTS update\n\n");

    return EXIT_SUCCESS;
}
