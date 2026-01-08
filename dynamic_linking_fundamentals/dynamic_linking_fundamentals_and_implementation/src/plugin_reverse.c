#include "plugin_api.h"
#include <string.h>
#include <stdio.h>

static int reverse_init(void) {
    printf("  [Plugin] String Reverser initialized\n");
    return 0;
}

static void reverse_cleanup(void) {
    printf("  [Plugin] String Reverser cleaned up\n");
}

static int reverse_process(const char *input, char *output, size_t out_size) {
    size_t len = strlen(input);
    if (len >= out_size) return -1;
    
    for (size_t i = 0; i < len; i++) {
        output[i] = input[len - 1 - i];
    }
    output[len] = '\0';
    return 0;
}

static plugin_api_t api = {
    .version = PLUGIN_API_VERSION,
    .name = "reverse",
    .description = "Reverses input strings",
    .init = reverse_init,
    .cleanup = reverse_cleanup,
    .process = reverse_process
};

/* Exported symbol for plugin loading */
plugin_api_t* get_plugin_interface(void) {
    return &api;
}
