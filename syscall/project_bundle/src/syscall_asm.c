#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

// Function that wraps a syscall - we'll inspect its disassembly
__attribute__((noinline))
long do_syscall_write(int fd, const void *buf, size_t count) {
    register long rax __asm__("rax") = SYS_write;
    register long rdi __asm__("rdi") = fd;
    register long rsi __asm__("rsi") = (long)buf;
    register long rdx __asm__("rdx") = count;
    register long ret;
    
    __asm__ __volatile__ (
        "syscall\n\t"
        : "=a"(ret)
        : "0"(rax), "r"(rdi), "r"(rsi), "r"(rdx)
        : "rcx", "r11", "memory"
    );
    
    return ret;
}

int main(void) {
    const char *msg = "Hello from syscall instruction!\n";
    do_syscall_write(STDOUT_FILENO, msg, 33);
    
    printf("\nTo see the actual SYSCALL instruction:\n");
    printf("  objdump -d build/syscall_asm | grep -A5 'syscall'\n\n");
    
    return 0;
}
