//
// Created by Leon on 2024/5/23.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <printf.h>
#include "ntc.h"
#include "log.h"

const char *suffix[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

char *xstrdup(const char *s) {
    char *t;
    t = strdup(s);
    if (!t) abort();
    return t;
}

void readable_size(double long bytes, char *result) {
    long double size = bytes;
    int i = 0;
    if (bytes > 0) {
        while (size >= 1024 && i < (sizeof(suffix) / sizeof(suffix[0])) - 1) {
            size /= 1024;
            i++;
        }
    }
    sprintf(result, "%.2Lf%s", size, suffix[i]);
}


long parse_time(const char *time_str) {
    long total_seconds = 0;
    long len = strlen(time_str);
    char *num_str = (char *)malloc(len); // 分配一个足够大的缓冲区
    int num_index = 0;

    for (int i = 0; i < len; i++) {
        if (isdigit(time_str[i])) {
            num_str[num_index++] = time_str[i];
        } else {
            num_str[num_index] = '\0'; // 终止字符串
            int value = atol(num_str); // 将数字字符串转换为长整型

            switch (time_str[i]) {
                case 's':
                    total_seconds += value;
                    break;
                case 'm':
                    total_seconds += value * 60;
                    break;
                case 'h':
                    total_seconds += value * 3600;
                    break;
                default:
                    fprintf(stderr, "Unknown time unit: %c\n", time_str[i]);
                    break;
            }
            num_index = 0; // 重置数字缓冲区索引
        }
    }

    free(num_str); // 释放缓冲区
    return total_seconds;
}

void generate_random_string(int length, char *random_string) {
    if (length <= 0) return;

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charset_size = sizeof(charset) - 1;

    srand(time(NULL));

    for (int i = 0; i < length; ++i) {
        random_string[i] = charset[rand() % charset_size];
    }
}