#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER "93.184.216.34"  // example.com
#define PORT 80
#define REQUEST "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n"

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[4096];
    ssize_t bytes;
    
    printf("\033[1;34m[Test Client]\033[0m Starting HTTP request to example.com...\n");
    
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    printf("\033[1;34m[Test Client]\033[0m Socket created (FD: %d)\n", sockfd);
    
    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER, &server_addr.sin_addr);
    
    // Connect
    printf("\033[1;34m[Test Client]\033[0m Connecting to %s:%d...\n", SERVER, PORT);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }
    
    printf("\033[1;32m[Test Client]\033[0m Connected!\n");
    sleep(1);
    
    // Send request
    printf("\033[1;34m[Test Client]\033[0m Sending HTTP request (%zu bytes)...\n", 
           strlen(REQUEST));
    if (send(sockfd, REQUEST, strlen(REQUEST), 0) < 0) {
        perror("send");
        close(sockfd);
        return 1;
    }
    
    sleep(1);
    
    // Receive response
    printf("\033[1;34m[Test Client]\033[0m Receiving response...\n");
    bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("\033[1;32m[Test Client]\033[0m Received %zd bytes\n", bytes);
        printf("\033[0;36m");
        printf("───── Response Preview ─────\n");
        printf("%.200s", buffer);
        if (bytes > 200) printf("\n... (truncated)");
        printf("\n────────────────────────────\n");
        printf("\033[0m");
    }
    
    sleep(1);
    
    // Close
    printf("\033[1;34m[Test Client]\033[0m Closing connection...\n");
    close(sockfd);
    
    printf("\033[1;32m[Test Client]\033[0m Complete!\n\n");
    
    return 0;
}
