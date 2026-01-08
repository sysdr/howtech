#include "plugin_api.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int rot13_init(void) {
    printf("  [Plugin] ROT13 Encoder initialized\n");
    return 0;
}

static void rot13_cleanup(void) {
    printf("  [Plugin] ROT13 Encoder cleaned up\n");
}

static int rot13_process(const char *input, char *output, size_t out_size) {
    size_t len = strlen(input);
    if (len >= out_size) return -1;
    
    for (size_t i = 0; i < len; i++) {
        char c = input[i];
        if (isalpha(c)) {
            char base = isupper(c) ? 'A' : 'a';
            output[i] = ((c - base + 13) % 26) + base;
        } else {
            output[i] = c;
        }
    }
    output[len] = '\0';
    return 0;
}

static plugin_api_t api = {
    .version = PLUGIN_API_VERSION,
    .name = "rot13",
    .description = "ROT13 cipher encoding",
    .init = rot13_init,
    .cleanup = rot13_cleanup,
    .process = rot13_process
};

plugin_api_t* get_plugin_interface(void) {
    return &api;
}
