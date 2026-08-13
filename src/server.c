#define _GNU_SOURCE

#include "server.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "html.h"
#include "syscall_wrappers.h"
#include "threadpool.h"

int conn_buf[CONN_QUEUE_SIZE] = {0};

int open_tcp_listener(char *port) {
    struct addrinfo hints = {0};
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Use TCP
    hints.ai_flags = AI_PASSIVE;     // Use localhost

    struct addrinfo *res = NULL;
    Getaddrinfo(NULL, port, &hints, &res);

    int listen_fd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    Bind(listen_fd, res->ai_addr, res->ai_addrlen);

    Listen(listen_fd, BACKLOG);

    freeaddrinfo(res);

    return listen_fd;
}

int spawn_workers(Server *s, size_t num_workers) {
    pthread_t tid;
    for (size_t i = 0; i < num_workers; i++) {
        if (Pthread_create(&tid, NULL, &worker_thread, &s->q) < 0) {
            return -1;
        }
        s->worker_tids[i] = tid;
    }
    return 0;
}

int server_init(Server *s, char *port) {
    s->listen_fd = open_tcp_listener(port);

    ConnectionQueue q;
    if (conn_queue_init(&q, conn_buf, CONN_QUEUE_SIZE) < 0) {
        return -1;
    }
    s->q = q;

    if (spawn_workers(s, N_WORKERS) < 0) {
        return -1;
    }

    s->running = true;

    return 0;
}

void server_run(Server *s) {
    struct sockaddr_storage conn_addr;
    socklen_t addr_len = sizeof(conn_addr);
    
    while (s->running) {
        int conn_fd = Accept(s->listen_fd, (struct sockaddr *)&conn_addr, &addr_len);
        if (conn_fd < 0) {
            continue;
        }

        conn_enque(&s->q, conn_fd);
    }
}