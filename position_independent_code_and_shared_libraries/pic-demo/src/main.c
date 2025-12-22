#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void library_function(void);
extern int get_global(void);
extern void modify_global(int val);

int main(int argc, char *argv[]) {
    printf("Main program starting (PID: %d)...\n", getpid());
    
    for (int i = 0; i < 3; i++) {
        library_function();
    }
    
    printf("Final global value: %d\n", get_global());
    
    // Keep alive for inspection if requested
    if (argc > 1 && strcmp(argv[1], "wait") == 0) {
        printf("Process waiting... Press Ctrl+C to exit\n");
        printf("Inspect with: cat /proc/%d/maps\n", getpid());
        getchar();
    }
    
    return 0;
}
