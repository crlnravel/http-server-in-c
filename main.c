#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <postgresql/libpq-fe.h>
#include "router.h"
#include "request.h"
#include "response.h"

// this server can handle up to ~2KB data at once
#define BUF_SIZE 2048
#define PORT 8080

// function for creating new route
route_trie_t* initiate_route() {
    route_trie_t *new = calloc(1, sizeof(route_trie_t));
    if (new == NULL) {
        perror("[-] memory error");
        exit(EXIT_FAILURE);
    }

    new->is_end = 1;

    // instantiate all null child
    for (int i = 0; i < MAX_SUBROUTE; i++) {
        new->subroutes[i] = NULL;
    }

    return new;
}

route_trie_t *route_trie_add(route_trie_t* p_trie, char* path, const int method, int (*handler)(int)) {
    route_trie_t *new =  initiate_route(path);

    int i = 0;

    // Add the new route if the parent isn't full;
    for (i = 0; i < MAX_SUBROUTE; i++) {
        if (p_trie->subroutes[i] == NULL) {
            p_trie->subroutes[i] = new;
            break;
        }
    }

    // Sorry bruv, you're hitting the limit
    if (i >= MAX_SUBROUTE) {
        perror("[!] Too many subroutes (sorry bruv, u're hitting the limit");
        exit(EXIT_FAILURE);
    }

    // change the parent trie
    p_trie->is_end = 0;

    new->method = method;
    new->path = path;
    new->handler = handler;

    return new;
}

void free_trie(route_trie_t *root) {
    if (root == NULL || root->is_end) {
        return;
    }

    for (int i = 0; i < MAX_SUBROUTE; i++) {
        if (root->subroutes[i] != NULL) {
            free_trie(root->subroutes[i]);
        }
    }

    // free the mem
    free(root);
    root = NULL;
}

int handle_route(const int client_sock, char *request_path, int method, route_trie_t *trie) {
    char* path[MAX_DEPTH_ROUTE] = { NULL };
    char* saveptr;
    char* token = strtok_r(request_path, "/", &saveptr);

    // split_path
    int i = 0;
    while (token != NULL) {
        // Add / to the beginning of each token
        char* tmp = malloc(sizeof(char));
        snprintf(tmp, strlen(token) + 2, "/%s", token);

        path[i] = tmp;
        i++;

        token = strtok_r(NULL, "/", &saveptr);
    }

    // map the path to the assigned route handler
    int j = 0;
    route_trie_t *cur = trie;
    while (path[j] != NULL || !cur->is_end) {
        int k = 0;
        for (; k < MAX_SUBROUTE; k++) {
            route_trie_t *sub = cur->subroutes[k];
            if (sub != NULL && strcmp(path[j], sub->path) == 0) {
                cur = sub;
                break;
            }
        }
        if (k >= MAX_SUBROUTE) {
            perror("[!] Route not found!");
            return -1;
        }   
        j++;
    }

    cur->handler(client_sock);

    return 0;
}

void url_decode(char *des, const char *src, const size_t src_len) {
    size_t decoded_len = 0;

    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            des[decoded_len++] = hex_val;
            i += 2;
        } else {
            des[decoded_len++] = src[i];
        }
    }

    des[decoded_len] = '\0';
}

request *parse_request(const char* buffer) {
    request *req = malloc(sizeof(request));

    // Setup regex for parsing url
    regex_t regex;
    regcomp(&regex, REQ_PATTERN, REG_EXTENDED);
    regmatch_t matches[3];

    // execute
    if (regexec(&regex, buffer, 3, matches, 0) != 0) {
        perror("[-] error parsing request");
        exit(EXIT_FAILURE);
    }
    FILE *f = fopen("kocak.txt", "w");

    // method in string
    // given that method can't be more than 6 character
    // method_str should only occupy 7 byte of mem including \0
    char *method_str = malloc(sizeof(char) * 7);
    char *path = malloc(sizeof(char) * (MAX_PATH_LENGTH + 1));

    if (method_str == NULL || path == NULL) {
        perror("[-] memory error");
        exit(EXIT_FAILURE);
    }

    bzero(method_str, 7);
    bzero(path, MAX_PATH_LENGTH + 1);

    strncpy(method_str, buffer + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
    strncpy(path, buffer + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);

    method_str[matches[1].rm_eo - matches[1].rm_so] = '\0';
    path[matches[2].rm_eo - matches[2].rm_so] = '\0';

    if (strcmp(method_str, "GET") == 0) req->method = GET;
    else if (strcmp(method_str, "POST") == 0) req->method = POST;
    else req->method = -1;

    req->path = path;

    fclose(f);

    free(method_str);
    regfree(&regex);

    return req;
}

void free_request(request *req) {
    free(req->path);
    free(req);
}

// TODO: FINISH THIS HANDLER
int get_all_handler(int client_sock) {
    char buffer[BUF_SIZE];

    FILE *f = fopen("myfile.json", "r");

    if (f == NULL) {
        perror("[-] file not found");
        exit(EXIT_FAILURE);
    }

    // find length of the file
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET); // set file back to beginning of the file

    char *file_buf = malloc(sizeof(char) * (size + 1));

    const size_t read_size = fread(file_buf, sizeof(char), size, f);

    if (size != read_size) {
        perror("[-] file error");
        exit(EXIT_FAILURE);
    }

    // set end of file_buf
    file_buf[size] = '\0';

    // template for OK response
    snprintf(buffer, BUF_SIZE, RESPONSE_TEMPLATE_OK, (int) size, "application/json");
    strcat(buffer, file_buf);

    fclose(f);
    free(file_buf);

    // send to client
    send(client_sock, buffer, BUF_SIZE, 0);
    return 0;
}

int main() {

    // server and client instance
    int server_sock;
    struct sockaddr_in  server_addr;

    // socket instantiation
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-] unable to create socket");
        exit(EXIT_FAILURE);
    }

    printf("[+] socket created\n");

    // finding free port 5 times
    int ii = 0;
    for (; ii < 5; ii++) {
        // configure server's socket
        server_addr.sin_family = AF_INET;  // IPV4
        server_addr.sin_port = htons(PORT + ii);  // configure port
        server_addr.sin_addr.s_addr = INADDR_ANY;  // bind to any address

        // bind server
        if ((bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
            printf("[!] unable to bind to port %d\n", PORT + ii);
        } else {
            break;
        }
    }
    if (ii == 5) exit(EXIT_FAILURE);

    // route handler
    route_trie_t *root = initiate_route();
    route_trie_t *tes = route_trie_add(root, "/tes", GET, *get_all_handler);
    route_trie_t *tes_kocak = route_trie_add(tes, "/kocak", GET, *get_all_handler);

    printf("[+] binding done\n");

    // listen to client's requests
    if (listen(server_sock, 10) < 0) {
        perror("[-] unable to listen");
        exit(EXIT_FAILURE);
    }

    printf("[+] listening to port %d...\n", (PORT + ii));

    for (;;) {
        // initiate client socket
        int *client_sock = malloc(sizeof(int));
        char *req_buf = malloc(sizeof(char) * BUF_SIZE);

        // check whether the client socket has been successfully created
        if (client_sock == NULL) {
            perror("[-] unable to allocate memory for client_sock");
            close(server_sock);  // don't forget to close the server's socket before closing
            exit(EXIT_FAILURE);
        }

        // client address
        struct sockaddr_in client_addr;
        socklen_t client_addr_length = sizeof(client_addr);

        // accept client connection if exists
        if ((*client_sock = accept(
                server_sock,
                (struct sockaddr *)&client_addr,
                (socklen_t *)&client_addr)) < 0) {
            perror("[-] unable to accept");
            exit(EXIT_FAILURE);
        }

        printf("[+] IP %s accepted\n", inet_ntoa(client_addr.sin_addr));

        if (recv(*client_sock, req_buf, BUF_SIZE, 0) < 0) {
            perror("[-] unable to receive request");
            exit(EXIT_FAILURE);
        }

        request *req = parse_request(req_buf);

        // // copy path to mutable char
        // char path_buf[MAX_PATH_LENGTH];
        // strcpy(path_buf, req->path);

        handle_route(*client_sock, req->path, req->method, root);

        free(client_sock);
        free(req_buf);
        free_request(req);
    }

    // close server at the end of life
    close(server_sock);

    // free route mem
    free_trie(root);

}