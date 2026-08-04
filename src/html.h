#pragma once

#include <sys/types.h>

typedef enum {
    GET,
} Method;

typedef enum {
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500,
    METHOD_NOT_IMPLEMENTED = 501,
    HTTP_VERSION_NOT_SUPPORTED = 505,
} HtmlStatus;

typedef struct {
    Method method;
    char *uri;
} Request;

char *html_status_msg(HtmlStatus status);

int parse_request(char *read_buf, size_t buf_size, Request *req_out);

char *handle_request(Request req, size_t *response_len);