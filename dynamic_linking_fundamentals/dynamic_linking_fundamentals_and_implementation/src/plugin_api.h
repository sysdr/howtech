#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stddef.h>

#define PLUGIN_API_VERSION 1

/* Plugin interface - ABI stable */
typedef struct {
    int version;
    const char *name;
    const char *description;
    
    /* Lifecycle functions */
    int (*init)(void);
    void (*cleanup)(void);
    
    /* Processing function */
    int (*process)(const char *input, char *output, size_t out_size);
} plugin_api_t;

/* Plugin must export this function */
typedef plugin_api_t* (*get_plugin_interface_fn)(void);

#endif /* PLUGIN_API_H */
