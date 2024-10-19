//
// Created by USER on 10/19/2024.
//

#ifndef REQUEST_H
#define REQUEST_H

#define GET 0
#define POST 1

#define REQ_PATTERN "^([A-Z]+) ([^ ]+) HTTP/1"

typedef struct request {
    char *path;
    int method;
} request;

request *parse_request(const char *buffer);

void free_request(request *req);

#endif //REQUEST_H
