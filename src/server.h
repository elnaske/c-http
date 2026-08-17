#pragma once

#include <stdbool.h>

#include "html.h"
#include "threadpool.h"

#define PORT "7878"
#define BACKLOG 10
#define READ_BUF_SIZE 1024
#define N_WORKERS 4
#define CONN_QUEUE_SIZE 16

typedef struct Server {
    ConnectionQueue q;
    size_t num_workers;
    pthread_t worker_tids[N_WORKERS];
    int listen_fd;
    bool running;
} Server;

int server_init(Server *s, char *port);

void server_run(Server *s);