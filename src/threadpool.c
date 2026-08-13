#include "threadpool.h"

#include <semaphore.h>
#include <stdint.h>

#include "syscall_wrappers.h"

int conn_queue_init(ConnectionQueue *q, int *conn_buf, uint32_t buf_size) {
    if (!q || !conn_buf) return -1;

    q->buf = conn_buf;
    q->buf_size = buf_size;
    q->front = q->back = 0;

    Sem_init(&q->mutex, 0, 1);
    Sem_init(&q->readable, 0, 0);

    // can fail if buf_size > SEM_VALUE_MAX
    if (Sem_init(&q->writeable, 0, buf_size) < 0) {
        return -1;
    }
    return 0;
}

int conn_enque(ConnectionQueue *q, int conn_fd) {
    sem_wait(&q->writeable);
    sem_wait(&q->mutex);

    q->buf[++(q->back) % (q->buf_size)] = conn_fd;

    sem_post(&q->mutex);
    sem_post(&q->readable);

    return 0;
}

int conn_deque(ConnectionQueue *q) {
    sem_wait(&q->readable);
    sem_wait(&q->mutex);

    int conn_fd = q->buf[++(q->front) % (q->buf_size)];

    sem_post(&q->mutex);
    sem_post(&q->writeable);

    return conn_fd;
}