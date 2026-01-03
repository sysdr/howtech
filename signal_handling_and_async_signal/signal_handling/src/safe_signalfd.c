#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>

// External Rust function - now safe to call from normal context
extern void rust_process_signal(int signal_num);

int main(void) {
    int sfd, epollfd;
    struct epoll_event ev, events[10];
    sigset_t mask;
    
    printf("Safe example using signalfd\n");
    printf("PID: %d\n\n", getpid());
    
    // Step 1: Block SIGUSR1 so it doesn't interrupt us
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        return 1;
    }
    
    printf("✓ Blocked signals (SIGUSR1, SIGINT, SIGTERM)\n");
    
    // Step 2: Create signalfd - converts signals to file descriptor events
    sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sfd < 0) {
        perror("signalfd");
        return 1;
    }
    
    printf("✓ Created signalfd: fd=%d\n", sfd);
    
    // Step 3: Create epoll instance for event loop
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd < 0) {
        perror("epoll_create1");
        close(sfd);
        return 1;
    }
    
    // Add signalfd to epoll
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) < 0) {
        perror("epoll_ctl");
        close(sfd);
        close(epollfd);
        return 1;
    }
    
    printf("✓ Integrated signalfd with epoll\n");
    printf("\nSend signals with: kill -USR1 %d\n", getpid());
    printf("Press Ctrl+C to exit gracefully\n\n");
    
    // Event loop - signals arrive as regular events
    int running = 1;
    int signal_count = 0;
    
    while (running) {
        printf("Waiting for events (signals will arrive as fd events)...\n");
        
        int nfds = epoll_wait(epollfd, events, 10, 5000);  // 5 second timeout
        
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        if (nfds == 0) {
            printf("  (timeout - no signals)\n");
            continue;
        }
        
        // Process events
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sfd) {
                // Signal arrived - read it from fd
                struct signalfd_siginfo si;
                ssize_t s = read(sfd, &si, sizeof(si));
                
                if (s != sizeof(si)) {
                    perror("read signalfd");
                    continue;
                }
                
                signal_count++;
                printf("\n✓ Received signal %d (total: %d)\n", si.ssi_signo, signal_count);
                
                if (si.ssi_signo == SIGINT || si.ssi_signo == SIGTERM) {
                    printf("  Shutdown signal received, exiting gracefully\n");
                    running = 0;
                } else {
                    printf("  Processing in normal context (safe!)...\n");
                    // NOW it's safe to call Rust - we're in normal execution context
                    rust_process_signal(si.ssi_signo);
                    printf("  ✓ Rust processing completed safely\n");
                }
            }
        }
    }
    
    printf("\nCleaning up...\n");
    close(sfd);
    close(epollfd);
    printf("Exited cleanly after processing %d signals\n", signal_count);
    
    return 0;
}
