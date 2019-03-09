#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_QUEUE 100  // Max amount of requests that can be pending for the listen call
#define BUF_SIZE 1024


int main(int argc, char **argv) {
    int sockfd;
    int connfd;
    int return_code;  // Variable for intermediate results
    ssize_t data_len;
    unsigned long port_num;

    socklen_t client_addr_len;

    char buf[BUF_SIZE];
    char *host;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;


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

    return_code = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (return_code != 0) {
        fprintf(stderr, "Bind failed\n");
        exit(EXIT_FAILURE);
    }

    return_code = listen(sockfd, MAX_QUEUE);
    if (return_code != 0) {
        fprintf(stderr, "Listen failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for incoming connections...\n");

    connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (connfd == -1) {
        fprintf(stderr, "Accept failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Accepted new connection from the client\nWaiting for data...\n");

    // Get (BUF_SIZE - 1) number of bytes.
    // Last byte of the buffer is used for the stop character.
    data_len = recv(connfd, buf, BUF_SIZE - 1, 0);
    if (data_len == -1) {
        fprintf(stderr, "Receive from the client failed\n");
        exit(EXIT_FAILURE);
    }

    buf[data_len] = '\0';

    printf("Received %ld bytes from the client\nData: \"%s\"\n", data_len, buf);

    close(connfd);
    close(sockfd);

    return 0;
}
