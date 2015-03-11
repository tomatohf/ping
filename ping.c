#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int get_addrinfo_v4(const char *host, struct addrinfo **result) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_CANONNAME;

    return getaddrinfo(host, NULL, &hints, result);
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

    char ip[INET_ADDRSTRLEN];
    inet_ntop(
        host->ai_family,
        &(((struct sockaddr_in *)host->ai_addr)->sin_addr),
        ip, sizeof(ip)
    );

    int sock = socket(host->ai_family, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
