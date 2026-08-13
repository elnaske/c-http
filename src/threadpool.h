#pragma once

#include <semaphore.h>
#include <stdint.h>

typedef struct Server Server;

typedef struct {
    int *buf;
    uint32_t buf_size;
    uint32_t front;
    uint32_t back;
    sem_t mutex;
    sem_t writeable;
    sem_t readable;
} ConnectionQueue;

int conn_queue_init(ConnectionQueue *q, int *conn_buf, uint32_t buf_size);

int conn_enque(ConnectionQueue *q, int conn_fd);

int conn_deque(ConnectionQueue *q);

void *worker_thread(void *arg);
