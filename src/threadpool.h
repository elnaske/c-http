#pragma once

#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Server Server;

typedef struct {
    int *buf;
    uint32_t buf_size;
    uint32_t front;
    uint32_t back;
    sem_t mutex;  // Restricts access to the queue
    sem_t spaces; // Notifies the main thread how many free spaces are in the queue
    sem_t items;  // Notifies the worker threads how many items are in the queue
    bool draining;// Notifies workers that the server is shutting down and to process remaining queued items
} ConnectionQueue;

int conn_queue_init(ConnectionQueue *q, int *conn_buf, uint32_t buf_size);

void conn_enque(ConnectionQueue *q, int conn_fd);

int conn_deque(ConnectionQueue *q);

void conn_queue_drain(ConnectionQueue *q, size_t num_workers);

void *worker_thread(void *arg);
