#include "syscall_wrappers.h"

#include <netdb.h>
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

int Socket(int domain, int type, int protocol) {
    int listen_fd = socket(domain, type, protocol);
    if (listen_fd < 0) {
        perror("Socket error");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
        perror("Setsockopt error");
        exit(1);
    }

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

int Recv(int fd, void *buf, size_t buf_size, int flags) {
    int bytes_read = recv(fd, buf, buf_size, flags);
    if (bytes_read < 0) {
        perror("Recv error");
        return -1;
    }
    return 0;
}