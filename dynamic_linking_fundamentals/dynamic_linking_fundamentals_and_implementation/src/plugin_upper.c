#include "plugin_api.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int upper_init(void) {
    printf("  [Plugin] Uppercase Converter initialized\n");
    return 0;
}

static void upper_cleanup(void) {
    printf("  [Plugin] Uppercase Converter cleaned up\n");
}

static int upper_process(const char *input, char *output, size_t out_size) {
    size_t len = strlen(input);
    if (len >= out_size) return -1;
    
    for (size_t i = 0; i < len; i++) {
        output[i] = toupper(input[i]);
    }
    output[len] = '\0';
    return 0;
}

static plugin_api_t api = {
    .version = PLUGIN_API_VERSION,
    .name = "upper",
    .description = "Converts text to uppercase",
    .init = upper_init,
    .cleanup = upper_cleanup,
    .process = upper_process
};

plugin_api_t* get_plugin_interface(void) {
    return &api;
}
