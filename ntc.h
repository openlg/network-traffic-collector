//
// Network traffic collection
// Created by Leon on 2024/5/23.
//

#ifndef NETWORK_TRAFFIC_COLLECTOR_NTC_H
#define NETWORK_TRAFFIC_COLLECTOR_NTC_H

#include <time.h>
#include <netinet/in.h>
#define HISTORY_LENGTH  20

typedef enum {
    THREAD_RUNNING,
    THREAD_FINISHED
} ThreadStatus;

typedef struct {
    double long total_sent;
    double long total_recv;
    time_t ts;
} Metrics;

struct ResponseHeaders {
    char http_version[16];
    int status_code;
    char status_text[64];
    char *content_type;
    long content_length;
};

typedef struct {
    char *data;
    size_t size;
    struct ResponseHeaders headers;
} Response;

int get_addrs_ioctl(char *interface, unsigned char if_hw_addr[], struct in_addr *if_ip_addr, struct in6_addr *if_ip6_addr);
char *xstrdup(const char *s);
void readable_size(double long bytes, char *result);
long parse_time(const char *time_str);

void generate_random_string(int length, char* random_string);
int contains(const char *haystack, const char *needle);

#endif //NETWORK_TRAFFIC_COLLECTOR_NTC_H
