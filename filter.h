//
// Created by Leon on 2024/5/25.
//

#ifndef NETWORK_TRAFFIC_FILTER_H
#define NETWORK_TRAFFIC_FILTER_H
int init_filter(const char *filter);

int filter_by_addr(char *addr);

void destroy_filter();

#endif //NETWORK_TRAFFIC_FILTER_H
