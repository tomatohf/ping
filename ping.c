#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define BUFFER_SIZE 1024

int get_addrinfo_v4(const char *host, struct addrinfo **result) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_CANONNAME;

    return getaddrinfo(host, NULL, &hints, result);
}

const char *get_sockaddr_text(
    const struct sockaddr *address,
    char *text, socklen_t text_length
) {
    return inet_ntop(
        address->sa_family,
        &(((struct sockaddr_in *)address)->sin_addr),
        text,
        text_length
    );
}

double timeval_to_ms(const struct timeval *time) {
    return (time->tv_sec * 1000.0) + (time->tv_usec / 1000.0);
}

int main(int argc, char **argv) {
    int status;

    if (argc != 2) {
        printf("usage: %s host\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct addrinfo *host;
    status = get_addrinfo_v4(argv[1], &host);
    if (status != 0) {
        printf("Error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sock = socket(host->ai_family, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    char send_ip[INET_ADDRSTRLEN];
    get_sockaddr_text(host->ai_addr, send_ip, sizeof(send_ip));

    struct ip ip_packet;
    struct sockaddr receive_address;
    char control_buffer[BUFFER_SIZE];

    struct iovec iov;
    iov.iov_base = &ip_packet;
    iov.iov_len = sizeof(ip_packet);

    struct msghdr msg;
    msg.msg_name = &receive_address;
    msg.msg_namelen = sizeof(receive_address);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control_buffer;
    msg.msg_controllen = sizeof(control_buffer);

    struct timeval receive_time;
    char receive_ip[INET_ADDRSTRLEN];

    for ( ; ; ) {
        ssize_t n = recvmsg(sock, &msg, 0);

        if (n > 0 && ip_packet.ip_p == IPPROTO_ICMP) {
            int ip_header_length = ip_packet.ip_hl << 2;
            int icmp_packet_length = n - ip_header_length;
            if (icmp_packet_length >= 16) {
                struct icmp *icmp_packet = (struct icmp *)(&ip_packet + ip_header_length);
                if (
                    icmp_packet->icmp_type == ICMP_ECHOREPLY &&
                    icmp_packet->icmp_id == (getpid() & 0xffff)
                ) {
                    gettimeofday(&receive_time, NULL);
                    struct timeval *send_time = (struct timeval *)icmp_packet->icmp_data;

                    printf(
                        "%d bytes from %s: icmp_seq=%u, ttl=%d, time=%.3f ms\n",
                        icmp_packet_length,
                        get_sockaddr_text(&receive_address, receive_ip, sizeof(receive_ip)),
                        icmp_packet->icmp_seq,
                        ip_packet.ip_ttl,
                        timeval_to_ms(&receive_time) - timeval_to_ms(send_time)
                    );
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
