//
// Created by Leon on 2024/6/3.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "./server.h"
#include "options.h"
#include "log.h"
#include "ntc.h"

extern options_t options;
extern Metrics metrics;
int stop_server = 0;

void start() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("set sock opt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(options.port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_socket, 10) < 0) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    stop_server = 0;
    log_info("HTTP server is running on port %d\n", options.port);

    while (!stop_server) {

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        struct timeval timeout = {1, 0};

        int activity = select(server_socket + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &readfds)) {
            client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client_socket < 0) {
                perror("accept");
                close(server_socket);
                exit(EXIT_FAILURE);
            }
            log_debug("Accept request");

            handle_client(client_socket);
        }
    }
    close(server_socket);
    log_info("Server is stopped.");
}

void handle_client(int client_socket) {
    char buffer[options.buffer_size];
    int bytes_received = read(client_socket, buffer, options.buffer_size - 1);

    if (bytes_received < 0) {
        perror("read");
        close(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';
    log_debug("Received request:\n%s\n", buffer);

    char html_headers[512] = {0};
    char html_content[1024] = {0};
    char last_capture_time[80];
    char send_str[20];
    char recv_str[20];

    strftime(last_capture_time, sizeof(last_capture_time), "%Y-%m-%d %H:%M:%S", localtime(&metrics.ts));

    readable_size(metrics.total_sent, send_str);
    readable_size(metrics.total_recv, recv_str);

    sprintf(html_content, "{"
                            "\"code\": \"ok\","
                            "\"data\": {"
                                "\"last_capture_time\": \"%s\","
                                "\"transmit\": \"%s\","
                                "\"received\": \"%s\","
                                "\"transmit_byte\": %Lf,"
                                "\"received_byte\": %Lf"
                            "}"
                          "}\r\n", last_capture_time, send_str, recv_str, metrics.total_sent, metrics.total_recv);
    sprintf(html_headers, "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: %lu\r\n"
                          "\r\n", strlen(html_content + 2));

    if (strncmp(buffer, "GET", 3) == 0) {
        write(client_socket, html_headers, strlen(html_headers));
        write(client_socket, html_content, strlen(html_content));
    } else {
        const char *error_message = "HTTP/1.1 501 Not Implemented\r\n\r\n";
        write(client_socket, error_message, strlen(error_message));
    }

    close(client_socket);
}

void stop() {
    stop_server = 1;
}