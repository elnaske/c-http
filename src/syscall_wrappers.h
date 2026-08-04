#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

int Socket(int domain, int type, int protocol);

int Bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int fd, int backlog);

int Recv(int fd, void *buf, size_t buf_size, int flags);