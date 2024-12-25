//
// Created by Leon on 2024/12/25.
//

#ifndef NETWORK_TRAFFIC_COLLECTOR_UTIL_H
#define NETWORK_TRAFFIC_COLLECTOR_UTIL_H
typedef struct {
    long     total;
    long     used;
    double    usage;
} Meter;
char *xstrdup(const char *s);
void readable_size(double long bytes, char *result);
long parse_time(const char *time_str);

void generate_random_string(int length, char* random_string);
int contains(const char *haystack, const char *needle);

Meter get_memory_meter();
Meter get_cpu_meter();
#endif //NETWORK_TRAFFIC_COLLECTOR_UTIL_H
