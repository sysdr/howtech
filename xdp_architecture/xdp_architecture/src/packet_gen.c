#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#define PACKET_SIZE 64
#define TARGET_PORT 9999
#define PACKETS_PER_SEC 100000

static volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/* Calculate checksum */
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    
    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <target_ip>\n", argv[0]);
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt");
        close(sock);
        return 1;
    }
    
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(argv[1]);
    
    char packet[PACKET_SIZE];
    struct iphdr *iph = (struct iphdr *)packet;
    struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
    char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    unsigned long packets_sent = 0;
    struct timeval start, now;
    gettimeofday(&start, NULL);
    
    printf("Sending UDP packets to %s:%d at %d pps\n", argv[1], TARGET_PORT, PACKETS_PER_SEC);
    printf("Press Ctrl+C to stop\n\n");
    
    while (running) {
        memset(packet, 0, PACKET_SIZE);
        
        /* Fill IP header */
        iph->ihl = 5;
        iph->version = 4;
        iph->tos = 0;
        iph->tot_len = htons(PACKET_SIZE);
        iph->id = htons(packets_sent & 0xFFFF);
        iph->frag_off = 0;
        iph->ttl = 64;
        iph->protocol = IPPROTO_UDP;
        iph->saddr = inet_addr("10.0.0.1");
        iph->daddr = dest.sin_addr.s_addr;
        iph->check = 0;
        iph->check = checksum(iph, sizeof(struct iphdr));
        
        /* Fill UDP header */
        udph->source = htons(12345);
        udph->dest = htons(TARGET_PORT);
        udph->len = htons(PACKET_SIZE - sizeof(struct iphdr));
        udph->check = 0;
        
        /* Fill data */
        snprintf(data, PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr), 
                 "XDP test packet %lu", packets_sent);
        
        if (sendto(sock, packet, PACKET_SIZE, 0, 
                   (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            if (errno != EINTR) {
                perror("sendto");
            }
            break;
        }
        
        packets_sent++;
        
        if (packets_sent % 10000 == 0) {
            gettimeofday(&now, NULL);
            double elapsed = (now.tv_sec - start.tv_sec) + 
                           (now.tv_usec - start.tv_usec) / 1000000.0;
            double pps = packets_sent / elapsed;
            printf("\rSent: %lu packets, Rate: %.0f pps", packets_sent, pps);
            fflush(stdout);
        }
        
        /* Rate limiting */
        if (packets_sent % 1000 == 0) {
            usleep(10000);  /* 10ms sleep every 1000 packets */
        }
    }
    
    printf("\n\nTotal packets sent: %lu\n", packets_sent);
    close(sock);
    return 0;
}
