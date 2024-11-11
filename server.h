#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define STATIC_DIR "./static"


typedef struct {
    int client_socket;
    struct sockaddr_in client_address;
} ClientData;


int start_server(int port);
void *client_thread(void *arg);
void handle_request(ClientData *client_data);

#endif 
