#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "7878"
#define BACKLOG 10
#define READ_BUF_SIZE 1024

int server_init() {
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    hints.ai_flags = AI_PASSIVE;     // Use localhost

    struct addrinfo *res = NULL;
    int status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "Getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listen_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("Setsockopt error");
        exit(1);
    }

    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Bind error");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("Listen error");
        exit(1);
    }

    freeaddrinfo(res);

    return listen_fd;
}

int handle_connection(int conn_fd) {
    char read_buf[READ_BUF_SIZE] = {0};
    int bytes_read = recv(conn_fd, &read_buf, READ_BUF_SIZE, 0);
    if (bytes_read < 0) {
        perror("Recv error");
        return -1;
    }

    // TODO: parse read_buf
    fprintf(stderr, "Request: %s\n", read_buf);

    char *msg = "HTTP/1.1 200 OK";

    int bytes_sent;
    int total_sent = 0;
    int bytes_remaining = strlen(msg);
    while ((bytes_sent = send(conn_fd, msg + total_sent, bytes_remaining, 0)) < bytes_remaining) {
        if (bytes_sent < 0) {
            perror("Send error");
            return -1;
        }

        total_sent += bytes_sent;
        bytes_remaining -= bytes_sent;
    }

    return 0;
}

void server_run(int listen_fd) {
    while (1) {
        struct sockaddr_storage conn_addr;
        socklen_t addr_len = sizeof(conn_addr);
        int conn_fd = accept(listen_fd, (struct sockaddr *)&conn_addr, &addr_len);
        if (conn_fd < 0) {
            perror("Accept error");
            continue;
        }
        fprintf(stderr, "Connection accepted\n");

        handle_connection(conn_fd);

        close(conn_fd);
        fprintf(stderr, "Connection closed\n");
    }
}

int main() {
    int listen_fd = server_init();

    server_run(listen_fd);
}