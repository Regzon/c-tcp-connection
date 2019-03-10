#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 20  // Max amount of clients that can communicate with the server at the same time
#define MAX_QUEUE 100   // Max amount of requests that can be pending for the listen call
#define BUF_SIZE 1024


pthread_t threads[MAX_CLIENTS];
int threads_availability[MAX_CLIENTS];
pthread_mutex_t threads_lock;

struct client_conn_data {
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
};


int get_thread_index() {
    pthread_mutex_lock(&threads_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (threads_availability[i] == 1) {
            pthread_mutex_unlock(&threads_lock);
            return i;
        }
    }

    pthread_mutex_unlock(&threads_lock);

    return -1;
}


int free_thread_index(pthread_t thread) {
    pthread_mutex_lock(&threads_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // If thread is not available and thread ids are matching
        if (threads_availability[i] == 0 && threads[i] == thread) {
            threads_availability[i] = 1;
            threads[i] = 0;
            pthread_mutex_unlock(&threads_lock);
            return 0;
        }
    }

    pthread_mutex_unlock(&threads_lock);

    return -1;
}


void *request_processing(void *thread_data) {
    struct client_conn_data *data = (struct client_conn_data *)thread_data;

    int return_code;
    ssize_t data_len;
    char buf[BUF_SIZE];

    // Get (BUF_SIZE - 1) number of bytes.
    // Last byte of the buffer is used for the stop character.
    data_len = recv(data->connfd, buf, BUF_SIZE - 1, 0);

    if (data_len == -1) {
        fprintf(stderr, "Receive from the client failed\n");
    }
    else {
        buf[data_len] = '\0';
        printf("Received %ld bytes from the client\nData: \"%s\"\n", data_len, buf);
    }

    return_code = free_thread_index(pthread_self());
    if (return_code == -1) {
        fprintf(stderr, "Thread release failed. Ignoring...\n");
    }

    close(data->connfd);
}


int main(int argc, char **argv) {
    int sockfd;
    int connfd;
    int return_code;   // Variable for intermediate results
    int thread_index;  // Variable for finding free thread index
    unsigned long port_num;

    char *host;

    socklen_t client_addr_len;


    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    struct client_conn_data *thread_data;


    if (argc != 3) {
        fprintf(stderr, "Invalid command format\nUsage: %s [host] [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Set all threads available by default
    for (int i = 0; i < MAX_CLIENTS; i++) threads_availability[i] = 1;

    host = argv[1];
    port_num = strtoul(argv[2], (char **)NULL, 10);

    printf("Setting up server...\n");

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

    while (1) {
        connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (connfd == -1) {
            fprintf(stderr, "Accept failed\n");
            continue;
        }

        printf("Accepted new connection from the client\nCreating thread...\n");

        thread_data = malloc(sizeof(struct client_conn_data));
        thread_data->connfd = connfd;
        thread_data->client_addr = client_addr;
        thread_data->client_addr_len = client_addr_len;

        while ((thread_index = get_thread_index()) == -1) {
            printf("All threads are busy, waiting...");
            sleep(1);
        }

        pthread_mutex_lock(&threads_lock);
        threads_availability[thread_index] = 0;
        pthread_create(&(threads[thread_index]), NULL, request_processing, (void *)thread_data);
        pthread_mutex_unlock(&threads_lock);
    }
    close(sockfd);

    return 0;
}
