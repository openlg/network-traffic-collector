//
// Created by Leon on 2024/5/25.
//

#include <string.h>
#include <printf.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>

regex_t *regex_array;
int regex_count = 0;

int init_filter(const char *filter) {
    if (filter != NULL) {
        unsigned long total_len = strlen(filter);
        char filter_copy[total_len + 1];
        strncpy(filter_copy, filter, total_len);
        filter_copy[total_len] = '\0';

        int pattern_count = 0;

        const char delim = ',';
        for (int i = 0; i < total_len; ++i) {
            if (filter_copy[i] == delim) {
                pattern_count++;
            }
        }

        //regex_t regex_array[pattern_count];
        regex_array = malloc(sizeof(regex_t) * pattern_count);
        if (regex_array == NULL) {
            printf("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        char *token = strtok(filter_copy, ",");
        int i = 0;
        while (token != NULL) {
            char regex_pattern[strlen(token) + 3];
            snprintf(regex_pattern, sizeof(regex_pattern), "^%s$", token);
            int reti = regcomp(&regex_array[i], regex_pattern, REG_EXTENDED);
            if (reti) {
                fprintf(stderr, "Could not compile regex: %s\n", regex_pattern);
                exit(EXIT_FAILURE);
            }
            i++;
            token = strtok(NULL, ",");
        }
        regex_count = i;
    }
    return regex_count;
}

void destroy_filter() {
    free(regex_array);
}

/**
 * 过滤指定地址
 * @param addr
 * @return 1 表示匹配上，需要过滤；0 表示没有匹配上，不需要过滤。
 */
int filter_by_addr(char *addr) {

    if (regex_array == NULL)
        return 0;

    for (int i = 0; i < regex_count; i++) {
        int reti = regexec(&regex_array[i], addr, 0, NULL, 0);
        if (!reti) {
            return 1;
        }
    }
    /*char *ret = strstr(options.filter, addr);
    return ret == NULL ? 0 : 1;*/
    return 0;
}