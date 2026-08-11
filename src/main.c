#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
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

int send_response(int conn_fd, Response res) {
    size_t response_len;
    char *serialized = serialize_reponse(res, &response_len);
    if (!serialized) {
        return -1;
    }

    int status = Send(conn_fd, serialized, response_len, 0);

    free(serialized);

    return status;
}

int handle_connection(int conn_fd) {
    char read_buf[READ_BUF_SIZE] = {0};
    if (Recv(conn_fd, &read_buf, READ_BUF_SIZE, 0) < 0) {
        return -1;
    }

    Request req;
    int status = parse_request(read_buf, READ_BUF_SIZE, &req);

    Response res;
    if (status == OK) {
        res = handle_request(req);
    } else {
        res = build_response(status, NULL);
    }

    if (send_response(conn_fd, res) < 0) {
        return -1;
    }

    return 0;
}

void *thread(void *arg) {
    int conn_fd = (int)(intptr_t)arg;
    pthread_t self_tid = pthread_self();

    pthread_detach(self_tid);

    fprintf(stderr, "[Thread #%ld] Connection accepted\n", self_tid);

    if (handle_connection(conn_fd) < 0) {
        send_response(conn_fd, build_response(INTERNAL_ERROR, NULL));
    }

    close(conn_fd);

    fprintf(stderr, "[Thread #%ld] Connection closed\n", self_tid);

    return NULL;
}

void server_run(int listen_fd) {
    while (1) {
        struct sockaddr_storage conn_addr;
        socklen_t addr_len = sizeof(conn_addr);
        int conn_fd = Accept(listen_fd, (struct sockaddr *)&conn_addr, &addr_len);
        if (conn_fd < 0) {
            continue;
        }

        pthread_t tid;
        Pthread_create(&tid, NULL, &thread, (void *)(intptr_t)conn_fd);
    }
}

int main() {
    int listen_fd = server_init();

    server_run(listen_fd);
}