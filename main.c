#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "router.h"

// this server can handle up to ~2KB data at once
#define BUF_SIZE 2048
#define PORT 8080

void *handler(void *arg) {
    int fd = *((int *)arg);
    char *buffer = malloc(BUF_SIZE * sizeof(char));

    if (buffer == NULL) {
        perror("[-] memory error");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = recv(fd, buffer, BUF_SIZE, 0);

    if (bytes_read < 0) {
        perror("[-] recv error");
        exit(EXIT_FAILURE);
    }

    if (bytes_read > 0) {
        regex_t

        buffer[bytes_read] = '\0';
    }

}

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

void route_trie_add(route_trie_t* p_trie, char* path, const int method, void (*handler)(int)) {
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
}

void free_trie(route_trie_t *root) {
    if (root == NULL) {
        return;
    }

    if (root->is_end) {
        free(root);
        root = NULL;
        return;
    }

    for (int i = 0; i < MAX_SUBROUTE; i++) {
        if (root->subroutes[i] != NULL) {
            free_trie(root->subroutes[i]);
        }
    }
}

void parse_route(char *request_path, route_trie_t *trie) {

    char* saveptr;
    char* token = strtok_r(request_path, " ", &saveptr);
    while (token != NULL) {
        if (!trie->is_end) {
            return;
        }

        for (int i = 0; i < MAX_SUBROUTE; i++) {
            if (trie->subroutes[i]->path == token) {
                trie = trie->subroutes[i];
            }
        }
        token = strtok_r(NULL, " ", &saveptr);
    }

    if (trie->handler != NULL) {
    }


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

    // configure server's socket
    server_addr.sin_family = AF_INET;  // IPV4
    server_addr.sin_port = htons(PORT);  // configure port
    server_addr.sin_addr.s_addr = INADDR_ANY;  // bind to any address

    // bind server
    if ((bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0) {
        perror("[-] unable to bind");
        exit(EXIT_FAILURE);
    }

    // route handler
    route_trie_t *root = initiate_route();
    route_trie_add(root, "/tes", GET);

    printf("[+] bind done\n");

    // listen to client's requests
    if (listen(server_sock, 10) < 0) {
        perror("[-] unable to listen");
        exit(EXIT_FAILURE);
    }

    printf("[+] listening...\n");

    for (;;) {
        // initiate client socket
        int *client_sock = malloc(sizeof(int));

        // check whether the client socket has bee successfully created
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

    }

    // close server at the end of life
    close(server_sock);

}