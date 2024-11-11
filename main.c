#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"

#define DEFAULT_PORT 80

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        if (opt == 'p') {
            port = atoi(optarg);
        }
    }

    
    int server_socket = start_server(port);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to start server\n");
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d...\n", port);

  
    while (1) {
        ClientData *client_data = malloc(sizeof(ClientData));
        socklen_t addr_len = sizeof(client_data->client_address);
        client_data->client_socket = accept(server_socket, (struct sockaddr *)&client_data->client_address, &addr_len);

        if (client_data->client_socket < 0) {
            perror("accept");
            free(client_data);
            continue;
        }

    
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_data);
        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}
