//
// Created by Leon on 2024/5/23.
//

#ifndef NETWORK_TRAFFIC_OPTIONS_H
#define NETWORK_TRAFFIC_OPTIONS_H

typedef struct {
    char *interface;

    char *url; /* push data to server url */
    long interval; /* seconds of push data interval */

    int promiscuous;
    int promiscuous_but_choosy;

    char *filter;
} options_t;

void options_set_defaults() ;
void options_read_args(int argc, char **argv);
int options_check();

#endif //NETWORK_TRAFFIC_OPTIONS_H
