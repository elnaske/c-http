#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "html.h"
#include "syscall_wrappers.h"

#define PORT "7878"
#define BACKLOG 10
#define READ_BUF_SIZE 1024

int server_init() {
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    hints.ai_flags = AI_PASSIVE;     // Use localhost

    struct addrinfo *res = NULL;
    Getaddrinfo(NULL, PORT, &hints, &res);

    int listen_fd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    Bind(listen_fd, res->ai_addr, res->ai_addrlen);

    Listen(listen_fd, BACKLOG);

    freeaddrinfo(res);

    return listen_fd;
}

int send_response(int conn_fd, char *response) {
    int bytes_sent;
    int total_sent = 0;
    int bytes_remaining = strlen(response);
    while ((bytes_sent = send(conn_fd, response + total_sent, bytes_remaining, 0)) < bytes_remaining) {
        if (bytes_sent < 0) {
            perror("Send error");
            return -1;
        }

        total_sent += bytes_sent;
        bytes_remaining -= bytes_sent;
    }
    return 0;
}

int handle_connection(int conn_fd) {
    char read_buf[READ_BUF_SIZE] = {0};
    if (Recv(conn_fd, &read_buf, READ_BUF_SIZE, 0) < 0) {
        return -1;
    }

    Request req;
    int status;
    if ((status = parse_request(read_buf, READ_BUF_SIZE, &req)) != OK) {
        send_response(conn_fd, html_status_msg(status));
        return 0;
    }

    size_t response_len;
    char *response = handle_request(req, &response_len);
    if (!response) {
        return -1;
    }

    send_response(conn_fd, response);

    free(response);

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

        if (handle_connection(conn_fd) < 0) {
            send_response(conn_fd, html_status_msg(INTERNAL_ERROR));
        }

        close(conn_fd);
        fprintf(stderr, "Connection closed\n");
    }
}

int main() {
    int listen_fd = server_init();

    server_run(listen_fd);
}