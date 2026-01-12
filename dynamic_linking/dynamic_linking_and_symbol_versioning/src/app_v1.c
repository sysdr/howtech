#include <stdio.h>

/* Declarations for v1.0 API */
extern int api_init(const char *config);
extern int api_process(const char *data);
extern void api_cleanup(void);

int main() {
    printf("Application compiled against MYLIB_1.0\n");
    printf("========================================\n\n");
    
    api_init("config_v1.ini");
    
    int result = api_process("Hello from v1 app");
    printf("Processed %d bytes\n\n", result);
    
    api_cleanup();
    
    return 0;
}
