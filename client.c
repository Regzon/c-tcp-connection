#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char **argv) {
    int sockfd;
    int return_code;  // Variable for intermediate results
    ssize_t data_len;
    unsigned long port_num;

    char *host;
    const char message[] = "Hello, Server! It's client";

    struct sockaddr_in server_addr;


    if (argc != 3) {
        fprintf(stderr, "Invalid command format\nUsage: %s [host] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    host = argv[1];
    port_num = strtoul(argv[2], (char **)NULL, 10);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host);
    server_addr.sin_port = htons(port_num);

    printf("Connecting to the server...\n");

    return_code = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (return_code != 0) {
        fprintf(stderr, "Connection to server failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Sending data to the server...\n");

    data_len = send(sockfd, message, sizeof(message), 0);
    if (data_len == -1) {
        fprintf(stderr, "Send to server failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Successfully sent %ld bytes to the server\n", data_len);

    close(sockfd);

    return 0;
}
