#include "threadpool.h"

#include <semaphore.h>
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
    char read_buf[READ_BUF_SIZE] = {0};
    if (Recv(conn_fd, &read_buf, READ_BUF_SIZE, 0) < 0) {
        return -1;
    }

    Request req;
    int status = parse_request(read_buf, READ_BUF_SIZE, &req);

    Response res;
    if (status == OK) {
        res = handle_request(req);
    } else {
        res = build_response(status, NULL);
    }

    if (send_response(conn_fd, res) < 0) {
        return -1;
    }

    return 0;
}

void *worker_thread(void *arg) {
    ConnectionQueue *q = arg;

    pthread_t self_tid = pthread_self();
    pthread_detach(self_tid);

    while (1) {
        int conn_fd = conn_deque(q);
        fprintf(stderr, "[Thread #%ld] Connection accepted\n", self_tid);

        if (handle_connection(conn_fd) < 0) {
            send_response(conn_fd, build_response(INTERNAL_ERROR, NULL));
        }

        close(conn_fd);
        fprintf(stderr, "[Thread #%ld] Connection closed\n", self_tid);
    }
}