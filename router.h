//
// Created by USER on 10/17/2024.
//

#ifndef ROUTER_H
#define ROUTER_H

#define BUF_SIZE 1024

/*
 * ik this sounds silly but like im too lazy to implement dynamic array alocation
 * for new child of the trie. why do i want to have >5 subroute anw :D
 * my mem will be doomed bruh :"V
 */
#define MAX_SUBROUTE 10
#define MAX_DEPTH_ROUTE 10

#define GET_PATTERN "^GET /([^ ]*) HTTP/1.1"

#define GET 0
#define POST 1
#define DELETE 2

/*
 * I am using trie implementation for the route handler. idk why it's just sound
 * more intuitive.
 */
typedef struct route_trie_t route_trie_t;

struct route_trie_t {
 char* path;
 route_trie_t* subroutes[MAX_SUBROUTE];
 short method;
 void (*handler)(int);
 int is_end;
};

// route_trie_method
route_trie_t* initiate_route();
void route_trie_add(route_trie_t* p_trie, char* path, int method);
void free_trie(route_trie_t* root);

void parse_request(char* request_path, route_trie_t* trie);

#endif //ROUTER_H
