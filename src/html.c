#include "html.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MIN(x, y) (x) < (y) ? (x) : (y)

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
    if (!read_buf || !req_out) return -1;

    size_t next_line_idx = 0;
    char *line = next_line(read_buf, buf_size, &next_line_idx);
    if (!line) {
        return -1;
    }

    fprintf(stderr, "Request: %s\n", line);

    Request req = {0};

    char *saveptr;
    char *method = strtok_r(line, " ", &saveptr);
    if (!method) return -1;

    if (strcmp("GET", method) == 0) {
        req.method = GET;
    } else {
        return -1;
    }

    char *uri = strtok_r(NULL, " ", &saveptr);
    if (!uri) return -1;
    req.uri = uri;

    char *protocol = strtok_r(NULL, " ", &saveptr);
    if (!protocol || strcmp("HTTP/1.1", protocol) != 0) {
        return -1;
    }

    char *remainder = strtok_r(NULL, " ", &saveptr);
    if (remainder) return -1; // no args left over

    *req_out = req;

    return 0;
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

char *build_response(char *msg, char *file, size_t *response_len) {
    size_t content_len;
    char *content = read_file(file, &content_len);
    if (!content) {
        return NULL;
    }

    size_t len = strlen(msg) + content_len + 64;
    char *response = malloc(len);
    if (!response) {
        return NULL;
    }

    snprintf(response, len, "%s\r\nContent-Length: %ld\r\n\r\n%s", msg, content_len, content);

    free(content);

    if (response_len) {
        *response_len = len;
    }
    return response;
}

char *handle_request(Request req, size_t *response_len) {
    char *response = NULL;

    if (strcmp("/", req.uri) == 0) {
        response = build_response("HTTP/1.1 200 OK", "web/hello.html", response_len);
    } else {
        response = build_response("HTTP/1.1 404 NOT FOUND", "web/404.html", response_len);
    }

    return response;
}