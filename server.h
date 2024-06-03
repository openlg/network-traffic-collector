//
// Created by Leon on 2024/6/3.
//

#ifndef NETWORK_TRAFFIC_COLLECTOR_SERVER_H
#define NETWORK_TRAFFIC_COLLECTOR_SERVER_H

void start();
void handle_client(int client_socket);
void stop();

#endif //NETWORK_TRAFFIC_COLLECTOR_SERVER_H
