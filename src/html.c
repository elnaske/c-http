#include "html.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MIN(x, y) (x) < (y) ? (x) : (y)

char *html_status_msg(HtmlStatus status) {
    switch (status) {
    case OK:
        return "HTTP/1.1 200 OK";
    case BAD_REQUEST:
        return "HTTP/1.1 400 BAD REQUEST";
    case NOT_FOUND:
        return "HTTP/1.1 404 NOT FOUND";
    case REQUEST_TIMEOUT:
        return "HTTP/1.1 408 REQUEST TIMEOUT";
    case INTERNAL_ERROR:
        return "HTTP/1.1 500 INTERNAL SERVER ERROR";
    case METHOD_NOT_IMPLEMENTED:
        return "HTTP/1.1 501 NOT IMPLEMENTED";
    case HTTP_VERSION_NOT_SUPPORTED:
        return "HTTP/1.1 505 HTML VERSION NOT SUPPORTED";
    default:
        return NULL;
    }
    return NULL;
}

static char *next_line(char *read_buf, size_t buf_size, size_t *start_idx) {
    if (!read_buf || !start_idx) return NULL;

    char *line = NULL;

    size_t idx = *start_idx;
    while (idx < buf_size && read_buf[idx] != '\r') {
        idx++;
    }

    if (idx + 1 < buf_size && read_buf[idx + 1] == '\n') {
        read_buf[idx] = '\0';
        line = read_buf + *start_idx;
        idx += 2;
    }

    *start_idx = idx;

    return line;
}

int parse_request(char *read_buf, size_t buf_size, Request *req_out) {
    if (!read_buf || !req_out) return INTERNAL_ERROR;

    size_t next_line_idx = 0;
    char *line = next_line(read_buf, buf_size, &next_line_idx);
    if (!line) {
        return INTERNAL_ERROR;
    }

    fprintf(stderr, "Request: %s\n", line);

    char *saveptr;
    char *method = strtok_r(line, " ", &saveptr);
    char *uri = strtok_r(NULL, " ", &saveptr);
    char *protocol = strtok_r(NULL, " ", &saveptr);
    char *remainder = strtok_r(NULL, " ", &saveptr); // no leftover args

    if (!method || !uri || !protocol || remainder) {
        return BAD_REQUEST;
    }

    if (strcmp(protocol, "HTTP/1.1") != 0) {
        return HTTP_VERSION_NOT_SUPPORTED;
    }

    Method m;
    if (strcmp(method, "GET") == 0) {
        m = GET;
    } else {
        return METHOD_NOT_IMPLEMENTED;
    }

    *req_out = (Request){
        .method = m,
        .uri = uri,
    };

    return OK;
}

char *read_file(char *file, size_t *size) {
    FILE *fp = fopen(file, "rb");
    if (!fp) {
        perror("File open error");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long l = ftell(fp);
    if (l < 0) {
        fclose(fp);
        return NULL;
    }
    size_t len = (size_t)l;

    rewind(fp);

    char *file_buf = malloc(len + 1);
    if (!file_buf) {
        return NULL;
    }

    size_t bytes_read = fread(file_buf, 1, len, fp);
    fclose(fp);

    if (bytes_read != len) {
        free(file_buf);
        return NULL;
    }

    file_buf[len] = '\0';

    if (size) {
        *size = len;
    }

    return file_buf;
}

Response build_response_from_status(HtmlStatus status, char *file) {
    return (Response){
        .status = status,
        .file = file,
    };
}

char *serialize_reponse(Response res, size_t *len_out) {
    char *status_msg = html_status_msg(res.status);
    size_t status_msg_len = strlen(status_msg);

    if (!res.file) {
        if (len_out) {
            *len_out = status_msg_len;
        }
        return strdup(status_msg); // strdup to avoid calling free() on static memory
    }

    size_t content_len;
    char *content = read_file(res.file, &content_len);
    if (!content) {
        return NULL;
    }

    size_t padding = 64;
    size_t response_len = status_msg_len + content_len + padding;
    char *response = malloc(response_len);
    if (!response) {
        return NULL;
    }

    snprintf(response, response_len, "%s\r\nContent-Length: %ld\r\n\r\n%s", status_msg, content_len, content);

    free(content);

    if (len_out) {
        *len_out = response_len;
    }
    return response;
}

Response handle_request(Request req) {
    Response res = {0};

    if (strcmp("/", req.uri) == 0) {
        res = build_response_from_status(OK, "web/index.html");
    } else if (strcmp("/sleep", req.uri) == 0) {
        sleep(5);
        res = build_response_from_status(OK, "web/index.html");
    } else {
        res = build_response_from_status(NOT_FOUND, "web/404.html");
    }

    return res;
}