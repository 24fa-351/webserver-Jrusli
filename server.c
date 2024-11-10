#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/stat.h>
//#include <mime.h>
#include "server.h"

int request_count = 0;
int received_bytes = 0;
int sent_bytes = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;


int start_server(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port),
    };

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 10) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    return server_socket;
}


void update_stats(int req_bytes, int res_bytes) {
    pthread_mutex_lock(&stats_mutex);
    request_count++;
    received_bytes += req_bytes;
    sent_bytes += res_bytes;
    pthread_mutex_unlock(&stats_mutex);
}


void send_response(int client_socket, const char *status, const char *content_type, const char *body) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n%s",
             status, content_type, strlen(body), body);
    send(client_socket, response, strlen(response), 0);
    update_stats(0, strlen(response));
}



void send_file_response(int client_socket, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        send_response(client_socket, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
        return;
    }

    struct stat file_stat;
    stat(file_path, &file_stat);

 
    char *mime_type = "application/octet-stream"; 
    const char *ext = strrchr(file_path, '.'); 
    if (ext) {
        if (strcmp(ext, ".png") == 0) {
            mime_type = "image/png";
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            mime_type = "image/jpeg";
        } else if (strcmp(ext, ".gif") == 0) {
            mime_type = "image/gif";
        } else if (strcmp(ext, ".html") == 0) {
            mime_type = "text/html";
        } else if (strcmp(ext, ".css") == 0) {
            mime_type = "text/css";
        } else if (strcmp(ext, ".js") == 0) {
            mime_type = "application/javascript";
        }

    }


    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", mime_type, file_stat.st_size);
    send(client_socket, response, strlen(response), 0);

    int sent = strlen(response);
    while (!feof(file)) {
        int bytes_read = fread(response, 1, sizeof(response), file);
        send(client_socket, response, bytes_read, 0);
        sent += bytes_read;
    }
    update_stats(0, sent);
    fclose(file);
}



void handle_request(ClientData *client_data) {
    int client_socket = client_data->client_socket;
    free(client_data);

    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    buffer[bytes_read] = '\0';

    if (bytes_read < 0) {
        close(client_socket);
        return;
    }

    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);


    update_stats(bytes_read, 0);

    if (strcmp(path, "/stats") == 0) {
        char stats_body[512];
        pthread_mutex_lock(&stats_mutex);
        snprintf(stats_body, sizeof(stats_body),
                 "<html><body><h1>Server Stats</h1><p>Requests: %d</p><p>Received Bytes: %d</p><p>Sent Bytes: %d</p></body></html>",
                 request_count, received_bytes, sent_bytes);
        pthread_mutex_unlock(&stats_mutex);
        send_response(client_socket, "200 OK", "text/html", stats_body);

    } else if (strncmp(path, "/static/", 8) == 0) {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s%s", STATIC_DIR, path + 7); // skip "/static/"
        send_file_response(client_socket, file_path);

    } else if (strncmp(path, "/calc?", 6) == 0) {
        int a, b;
        sscanf(path, "/calc?a=%d&b=%d", &a, &b);
        char calc_body[256];
        snprintf(calc_body, sizeof(calc_body), "<html><body><h1>Calculation Result</h1><p>%d + %d = %d</p></body></html>", a, b, a + b);
        send_response(client_socket, "200 OK", "text/html", calc_body);

    } else {
        send_response(client_socket, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
    }

    close(client_socket);
}


void *client_thread(void *arg) {
    handle_request((ClientData *)arg);
    return NULL;
}
