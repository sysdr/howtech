#include <stdio.h>
#include <unistd.h>

int main(void) {
    for (int i = 0; i < 5; i++) {
        pid_t pid = getpid();
        printf("PID: %d\n", pid);
    }
    return 0;
}
