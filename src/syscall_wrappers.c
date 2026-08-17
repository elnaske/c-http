#include "syscall_wrappers.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

int Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    int status = getaddrinfo(node, service, hints, res);
    if (status != 0) {
        fprintf(stderr, "Getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    return 0;
}

int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    if (setsockopt(fd, level, optname, optval, optlen) < 0) {
        perror("Setsockopt error");
        exit(1);
    }
    return 0;
}

int Socket(int domain, int type, int protocol) {
    int listen_fd = socket(domain, type, protocol);
    if (listen_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    int optval = 1;
    Setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    return listen_fd;
}

int Bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
    if (bind(fd, addr, addrlen) < 0) {
        perror("Bind error");
        exit(1);
    }
    return 0;
}

int Listen(int fd, int backlog) {
    if (listen(fd, backlog) < 0) {
        perror("Listen error");
        exit(1);
    }
    return 0;
}

int Accept(int fd, struct sockaddr *conn_addr, socklen_t *addr_len) {
    int conn_fd = accept(fd, conn_addr, addr_len);
    if (conn_fd < 0 && errno != EINTR) { // interrupts should not print an error msg
        perror("Accept error");
        return -1;
    }
    return conn_fd;
}

int Recv(int fd, void *buf, size_t buf_size, int flags) {
    int bytes_read = recv(fd, buf, buf_size, flags);
    if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("Recv error");
        return -1;
    }
    return 0;
}

int Send(int fd, const void *buf, size_t n, int flags) {
    int bytes_sent;
    int total_sent = 0;
    int bytes_remaining = n;
    while ((bytes_sent = send(fd, (char *)buf + total_sent, bytes_remaining, flags)) < bytes_remaining) {
        if (bytes_sent < 0) {
            perror("Send error");
            return -1;
        }

        total_sent += bytes_sent;
        bytes_remaining -= bytes_sent;
    }
    return 0;
}

int Pthread_create(pthread_t *tid, const pthread_attr_t *attr, void *(start_routine)(void *), void *arg) {
    int status = pthread_create(tid, attr, start_routine, arg);
    if (status != 0) {
        errno = status;
        perror("Failed to create thread");
        return -1;
    }
    return 0;
}

int Sem_init(sem_t *sem, int pshared, unsigned int value) {
    if (sem_init(sem, pshared, value) < 0) {
        perror("Failed to initialize semaphore");
        return -1;
    }
    return 0;
}