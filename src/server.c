#define _GNU_SOURCE

#include "server.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "html.h"
#include "shutdown.h"
#include "syscall_wrappers.h"
#include "threadpool.h"

#define INACTIVITY_TIMEOUT_SECS 5
#define INACTIVITY_TIMEOUT_USECS 0

extern volatile sig_atomic_t shutdown_requested;
int conn_buf[CONN_QUEUE_SIZE] = {0};

void set_socket_timeout(int fd, time_t secs, suseconds_t usecs) {
    struct timeval timeout;
    timeout.tv_sec = secs;
    timeout.tv_usec = usecs;

    Setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

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
    for (size_t i = 0; i < num_workers; i++) {
        pthread_t tid;
        if (Pthread_create(&tid, NULL, &worker_thread, &s->q) < 0) {
            return -1;
        }
        s->worker_tids[i] = tid;
    }
    s->num_workers = num_workers;
    return 0;
}

int server_init(Server *s, char *port) {
    s->listen_fd = open_tcp_listener(port);

    ConnectionQueue q;
    if (conn_queue_init(&q, conn_buf, CONN_QUEUE_SIZE) < 0) {
        return -1;
    }
    s->q = q;

    install_sigint_handler(request_shutdown);

    if (spawn_workers(s, N_WORKERS) < 0) {
        return -1;
    }

    s->running = true;

    return 0;
}

void server_run(Server *s) {
    struct sockaddr_storage conn_addr;
    socklen_t addr_len = sizeof(conn_addr);

    // while (s->running) {
    while (!shutdown_requested) {
        int conn_fd = Accept(s->listen_fd, (struct sockaddr *)&conn_addr, &addr_len);
        if (conn_fd < 0) {
            continue;
        }

        set_socket_timeout(conn_fd, INACTIVITY_TIMEOUT_SECS, INACTIVITY_TIMEOUT_USECS);

        conn_enque(&s->q, conn_fd);
    }

    fprintf(stderr, "Shutting down\n");

    fprintf(stderr, "Notifying workers\n");
    /* Currently, we only need to shutdown the workers, so the flag for doing so is internal to the queue.
     * This is nice because the mutex is only accessed within the queue.
     * If more features are added in the future, a global variable for notifying _all_ thread may be
     * a simpler solution.
     */
    conn_queue_drain(&s->q, s->num_workers);

    fprintf(stderr, "Joining worker threads\n");
    for (size_t i = 0; i < s->num_workers; i++) {
        int status = pthread_join(s->worker_tids[i], NULL);
        if (status != 0) {
            errno = status;
            perror("Join error");
        }
    }
}