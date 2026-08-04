#pragma once

#include <sys/types.h>

typedef enum {
    GET
} Method;

typedef struct {
    Method method;
    char *uri;
} Request;

int parse_request(char *read_buf, size_t buf_size, Request *req_out);

char *handle_request(Request req, size_t *response_len);