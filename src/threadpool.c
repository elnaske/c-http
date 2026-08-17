#include "threadpool.h"

#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "html.h"
#include "server.h"
#include "syscall_wrappers.h"

int conn_queue_init(ConnectionQueue *q, int *conn_buf, uint32_t buf_size) {
    if (!q || !conn_buf) return -1;

    q->buf = conn_buf;
    q->buf_size = buf_size;
    q->front = q->back = 0;

    Sem_init(&q->mutex, 0, 1);

    Sem_init(&q->items, 0, 0);
    if (Sem_init(&q->spaces, 0, buf_size) < 0) {
        // can fail if buf_size > SEM_VALUE_MAX
        return -1;
    }
    return 0;
}

// During regular operation, the items semaphore implicitly tells us if the queue is empty.
// We only need this function to explicitly check during shutdown (see conn_queue() below).
static inline int conn_queue_is_empty(ConnectionQueue *q) {
    return q->front == q->back;
}

void conn_enque(ConnectionQueue *q, int conn_fd) {
    sem_wait(&q->spaces);

    sem_wait(&q->mutex);

    // The spaces semaphore ensures that we can't enqueue more than the max number of items.
    // Hence no explicit overflow checks are necessary.
    q->buf[++(q->back) % (q->buf_size)] = conn_fd;
    sem_post(&q->mutex);

    sem_post(&q->items);
}

int conn_deque(ConnectionQueue *q) {
    sem_wait(&q->items);

    sem_wait(&q->mutex);

    /* During regular operation, the items semaphore ensures that we can only
     * deque when the queue is _not_ empty.
     * However, during shutdown the main thread will post the semaphore to wake up
     * the workers (Otherwise they could block indefinitely on a wait).
     * When this happens, we process the remaining requests until the queue is empty.
     */
    if (q->draining && conn_queue_is_empty(q)) {
        sem_post(&q->mutex);
        return -1;
    }

    int conn_fd = q->buf[++(q->front) % (q->buf_size)];
    sem_post(&q->mutex);

    sem_post(&q->spaces);

    return conn_fd;
}

void conn_queue_drain(ConnectionQueue *q, size_t num_workers) {
    // Notify workers
    sem_wait(&q->mutex);
    q->draining = true;
    sem_post(&q->mutex);

    // Wake up blocking workers waiting on the queue
    for (size_t i = 0; i < num_workers; i++) {
        sem_post(&q->items);
    }
}

int send_response(int conn_fd, Response res) {
    size_t response_len;
    char *serialized = serialize_reponse(res, &response_len);
    if (!serialized) {
        return -1;
    }

    int status = Send(conn_fd, serialized, response_len, 0);

    free(serialized);

    return status;
}

int handle_connection(int conn_fd) {
    Request req;
    Response res;
    int status;

    char read_buf[READ_BUF_SIZE] = {0};
    if (Recv(conn_fd, &read_buf, READ_BUF_SIZE, 0) < 0) {
        return -1;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        fprintf(stderr, "[Thread #%ld] Request timed out\n", pthread_self());
        status = REQUEST_TIMEOUT;
    } else {
        status = parse_request(read_buf, READ_BUF_SIZE, &req);
    }

    if (status == OK) {
        res = handle_request(req);
    } else {
        res = build_response_from_status(status, NULL);
    }

    if (send_response(conn_fd, res) < 0) {
        return -1;
    }

    return 0;
}

void *worker_thread(void *arg) {
    ConnectionQueue *q = arg;

    pthread_t self_tid = pthread_self();

    while (1) {
        int conn_fd = conn_deque(q);
        if (conn_fd < 0) {
            break;
        }

        fprintf(stderr, "[Thread #%ld] Connection accepted\n", self_tid);

        if (handle_connection(conn_fd) < 0) {
            send_response(conn_fd, build_response_from_status(INTERNAL_ERROR, NULL));
        }

        close(conn_fd);
        fprintf(stderr, "[Thread #%ld] Connection closed\n", self_tid);
    }

    return NULL;
}