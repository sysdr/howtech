#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdint.h>
#include <x86intrin.h>
#include "plugin_api.h"

#define MAX_PLUGINS 10
#define MAX_PATH 256

typedef struct {
    void *handle;
    plugin_api_t *api;
    char path[MAX_PATH];
    uint64_t load_cycles;
} loaded_plugin_t;

static loaded_plugin_t plugins[MAX_PLUGINS];
static int plugin_count = 0;

/* High-precision timing using RDTSC */
static inline uint64_t rdtsc(void) {
    return __rdtsc();
}

/* Load plugin with specified binding mode */
static int load_plugin(const char *path, int flags) {
    if (plugin_count >= MAX_PLUGINS) {
        fprintf(stderr, "Maximum plugin limit reached\n");
        return -1;
    }
    
    printf("\n%s[Loading]%s %s ", "\033[1;33m", "\033[0m", path);
    printf("(flags: %s)\n", 
           (flags & RTLD_NOW) ? "RTLD_NOW" : "RTLD_LAZY");
    
    /* Measure load time */
    uint64_t start = rdtsc();
    void *handle = dlopen(path, flags);
    uint64_t end = rdtsc();
    
    if (!handle) {
        fprintf(stderr, "%s[ERROR]%s dlopen failed: %s\n", 
                "\033[0;31m", "\033[0m", dlerror());
        return -1;
    }
    
    /* Clear any previous errors */
    dlerror();
    
    /* Get plugin interface */
    get_plugin_interface_fn get_interface = 
        (get_plugin_interface_fn)dlsym(handle, "get_plugin_interface");
    
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "%s[ERROR]%s dlsym failed: %s\n", 
                "\033[0;31m", "\033[0m", error);
        dlclose(handle);
        return -1;
    }
    
    plugin_api_t *api = get_interface();
    if (!api) {
        fprintf(stderr, "%s[ERROR]%s Plugin returned NULL interface\n", 
                "\033[0;31m", "\033[0m");
        dlclose(handle);
        return -1;
    }
    
    /* Version check */
    if (api->version != PLUGIN_API_VERSION) {
        fprintf(stderr, "%s[ERROR]%s Plugin version mismatch: %d != %d\n",
                "\033[0;31m", "\033[0m", api->version, PLUGIN_API_VERSION);
        dlclose(handle);
        return -1;
    }
    
    /* Initialize plugin */
    if (api->init && api->init() != 0) {
        fprintf(stderr, "%s[ERROR]%s Plugin initialization failed\n",
                "\033[0;31m", "\033[0m");
        dlclose(handle);
        return -1;
    }
    
    /* Store plugin info */
    loaded_plugin_t *p = &plugins[plugin_count];
    p->handle = handle;
    p->api = api;
    p->load_cycles = end - start;
    strncpy(p->path, path, MAX_PATH - 1);
    p->path[MAX_PATH - 1] = '\0';
    
    printf("  %s[OK]%s Loaded '%s': %s\n", 
           "\033[0;32m", "\033[0m", api->name, api->description);
    printf("  Load time: %lu CPU cycles (~%.2f µs @ 2.4GHz)\n\n",
           p->load_cycles, (double)p->load_cycles / 2400.0);
    
    plugin_count++;
    return 0;
}

/* Show memory mappings */
static void show_memory_maps(void) {
    printf("\n%s[Memory Mappings]%s\n", "\033[1;36m", "\033[0m");
    printf("%-20s %-18s %-10s %s\n", "Address Range", "Perms", "Offset", "Path");
    printf("%.80s\n", "================================================================================");
    
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        perror("fopen /proc/self/maps");
        return;
    }
    
    char line[512];
    int plugin_line = 0;
    while (fgets(line, sizeof(line), maps)) {
        /* Highlight plugin .so files */
        if (strstr(line, "plugin_") || strstr(line, ".so")) {
            if (!plugin_line) {
                printf("%s", "\033[1;32m");
                plugin_line = 1;
            }
            printf("%s", line);
        } else {
            if (plugin_line) {
                printf("%s", "\033[0m");
                plugin_line = 0;
            }
            /* Only show key mappings */
            if (strstr(line, "[heap]") || strstr(line, "[stack]") || 
                strstr(line, "libc") || strstr(line, "ld-")) {
                printf("%s", line);
            }
        }
    }
    if (plugin_line) printf("%s", "\033[0m");
    fclose(maps);
    printf("\n");
}

/* Test all plugins with timing */
static void test_plugins(void) {
    const char *test_input = "Hello Dynamic Loading!";
    char output[256];
    
    printf("%s[Testing Plugins]%s\n\n", "\033[1;36m", "\033[0m");
    
    for (int i = 0; i < plugin_count; i++) {
        plugin_api_t *api = plugins[i].api;
        
        printf("Plugin: %s%-10s%s ", "\033[1;33m", api->name, "\033[0m");
        
        /* Measure process call time */
        uint64_t total_cycles = 0;
        int iterations = 1000;
        
        for (int j = 0; j < iterations; j++) {
            uint64_t start = rdtsc();
            api->process(test_input, output, sizeof(output));
            uint64_t end = rdtsc();
            total_cycles += (end - start);
        }
        
        double avg_ns = ((double)total_cycles / iterations) / 2.4;
        
        printf("Input:  \"%s\"\n", test_input);
        printf("%-24sOutput: \"%s\"\n", "", output);
        printf("%-24sAvg call time: %.1f ns (%d iterations)\n\n",
               "", avg_ns, iterations);
    }
}

/* Unload all plugins */
static void cleanup_plugins(void) {
    printf("%s[Cleanup]%s Unloading plugins...\n\n", 
           "\033[1;33m", "\033[0m");
    
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i].api && plugins[i].api->cleanup) {
            plugins[i].api->cleanup();
        }
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
    }
    
    plugin_count = 0;
}

int main(int argc, char **argv) {
    int rtld_mode = RTLD_NOW | RTLD_LOCAL;
    
    printf("\n%s", "\033[1;36m");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Dynamic Module Loading Demonstration                    ║\n");
    printf("║  Showing: dlopen(), symbol resolution, and PLT/GOT       ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("%s\n", "\033[0m");
    
    /* Allow user to choose binding mode */
    if (argc > 1 && strcmp(argv[1], "--lazy") == 0) {
        rtld_mode = RTLD_LAZY | RTLD_LOCAL;
        printf("Using %sRTLD_LAZY%s binding (symbols resolved on first call)\n\n", 
               "\033[1;33m", "\033[0m");
    } else {
        printf("Using %sRTLD_NOW%s binding (all symbols resolved at load)\n\n",
               "\033[1;33m", "\033[0m");
    }
    
    /* Load plugins */
    load_plugin("./plugins/plugin_reverse.so", rtld_mode);
    load_plugin("./plugins/plugin_rot13.so", rtld_mode);
    load_plugin("./plugins/plugin_upper.so", rtld_mode);
    
    /* Show memory layout after loading */
    show_memory_maps();
    
    /* Test plugins */
    test_plugins();
    
    /* Cleanup */
    cleanup_plugins();
    
    /* Show memory after unloading */
    printf("\n%s[After dlclose()]%s Checking if plugins are unmapped...\n", 
           "\033[1;33m", "\033[0m");
    
    FILE *maps = fopen("/proc/self/maps", "r");
    char line[512];
    int still_mapped = 0;
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "plugin_")) {
            if (!still_mapped) {
                printf("\n%s[WARNING]%s Plugins still in memory:\n",
                       "\033[1;31m", "\033[0m");
            }
            printf("  %s", line);
            still_mapped = 1;
        }
    }
    fclose(maps);
    
    if (!still_mapped) {
        printf("%s[OK]%s All plugins successfully unmapped\n",
               "\033[0;32m", "\033[0m");
    }
    
    printf("\n%sDone!%s Run with --lazy to see lazy binding behavior\n\n",
           "\033[1;32m", "\033[0m");
    
    return 0;
}
