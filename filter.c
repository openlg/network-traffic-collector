//
// Created by Leon on 2024/5/25.
//

#include <string.h>
#include <printf.h>
#include <stdlib.h>
#include "log.h"

char **pattern_array = NULL;
int pattern_count = 0;

int init_filter(const char *filter) {
    if (filter != NULL) {
        unsigned long total_len = strlen(filter);
        char filter_copy[total_len + 1];
        strncpy(filter_copy, filter, total_len);
        filter_copy[total_len] = '\0';

        const char delim = ',';
        for (int i = 0; i < total_len; ++i) {
            if (filter_copy[i] == delim) {
                pattern_count++;
            }
        }

        pattern_array = malloc(pattern_count * sizeof(char *));

        char *token = strtok(filter_copy, ",");
        int i = 0;
        while (token != NULL) {
            pattern_array[i] = strdup(token);
            log_info("Match pattern: %s", pattern_array[i]);
            token = strtok(NULL, ",");
            i++;
        }
        pattern_count = i;
    }
    return pattern_count;
}

/**
 * 匹配字符串，pattern 支持使用 * 字符匹配任意字符
 * @param pattern
 * @param str
 * @return
 */
int match(char *pattern, char *str) {
    const char *p = pattern;
    const char *t = str;
    const char *star = NULL;
    const char *ss = str;

    while (*t) {
        if (*p == '?' || *p == *t) {
            p++;
            t++;
        } else if (*p == '*') {
            star = p++;
            ss = t;
        } else if (star) {
            p = star + 1;
            t = ++ss;
        } else {
            return 0;
        }
    }

    while (*p == '*') {
        p++;
    }

    return *p == '\0';
}

void destroy_filter() {
    free(pattern_array);
    pattern_array = NULL;
    pattern_count = 0;
}

/**
 * 过滤指定地址
 * @param addr
 * @return 1 表示匹配上，需要过滤；0 表示没有匹配上，不需要过滤。
 */
int filter_by_addr(char *addr) {

    if (pattern_array == NULL || pattern_count == 0)
        return 0;

    for (int i = 0; i < pattern_count; i++) {
        if (match(pattern_array[i], addr)) {
            return 1;
        }
    }
    return 0;
}