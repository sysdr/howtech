#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <link.h>

#define COLOR_HEADER "\033[1;36m"
#define COLOR_LIB    "\033[1;33m"
#define COLOR_SYM    "\033[0;32m"
#define COLOR_RESET  "\033[0m"

static int callback(struct dl_phdr_info *info, size_t size, void *data) {
    (void)size;
    (void)data;
    if (info->dlpi_name && info->dlpi_name[0] != '\0') {
        printf("  %s%-50s%s @ %s0x%lx%s\n",
               COLOR_LIB, info->dlpi_name, COLOR_RESET,
               COLOR_SYM, info->dlpi_addr, COLOR_RESET);
    }
    return 0;
}

int main() {
    printf("\n%s", COLOR_HEADER);
    printf("╔═════════════════════════════════════════════════════════╗\n");
    printf("║        Symbol Resolution Order Demonstration           ║\n");
    printf("╚═════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    printf("%sLoaded shared libraries (search order):%s\n\n", COLOR_HEADER, COLOR_RESET);
    dl_iterate_phdr(callback, NULL);
    
    printf("\n%s", COLOR_HEADER);
    printf("╔═════════════════════════════════════════════════════════╗\n");
    printf("║  Resolution algorithm:                                  ║\n");
    printf("║  1. LD_PRELOAD (highest priority)                       ║\n");
    printf("║  2. Binary's DT_RPATH (if no RUNPATH)                   ║\n");
    printf("║  3. LD_LIBRARY_PATH                                     ║\n");
    printf("║  4. Binary's DT_RUNPATH                                 ║\n");
    printf("║  5. /etc/ld.so.cache                                    ║\n");
    printf("║  6. /lib, /usr/lib (fallback)                           ║\n");
    printf("╚═════════════════════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);
    
    // Show address of printf in GOT
    void *printf_addr = dlsym(RTLD_DEFAULT, "printf");
    printf("%sAddress of printf():%s %s%p%s\n",
           COLOR_HEADER, COLOR_RESET, COLOR_SYM, printf_addr, COLOR_RESET);
    
    return 0;
}
