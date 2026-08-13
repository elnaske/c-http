#pragma once

#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>

int Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

int Socket(int domain, int type, int protocol);

int Bind(int fd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int fd, int backlog);

int Accept(int fd, struct sockaddr *conn_addr, socklen_t *addr_len);

int Recv(int fd, void *buf, size_t buf_size, int flags);

int Send(int fd, const void *buf, size_t n, int flags);

int Pthread_create(pthread_t *tid, const pthread_attr_t *attr, void *(start_routine)(void *), void *arg);

int Sem_init(sem_t *sem, int pshared, unsigned int value);