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

typedef struct {
    HtmlStatus status;
    char *file;
} Response;

char *html_status_msg(HtmlStatus status);

Response build_response(HtmlStatus status, char *file);

char *serialize_reponse(Response res, size_t *len_out);

int parse_request(char *read_buf, size_t buf_size, Request *req_out);

Response handle_request(Request req);