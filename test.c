//
// Created by Leon on 2024/5/29.
//
#include <stdio.h>
#include <assert.h>
#include <regex.h>
#include "filter.h"


int match_regex(const char *pattern, const char *text) {
    regex_t regex;
    int reti;

    reti = regcomp(&regex, pattern, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Could not compile regex\n");
        return 0;
    }

    reti = regexec(&regex, text, 0, NULL, 0);
    int result;
    if (!reti) {
        //printf("Match %d\n", reti);
        result = 1;
    } else if (reti == REG_NOMATCH) {
        //printf("No Match %d\n", reti);
        result = 0;
    } else {
        char msgbuf[100];
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        result = -1;
    }

    regfree(&regex);
    return result;
}

void test_regex_match() {
    assert(match_regex("192\\.168\\.*", "192.168.1.1") == 1);
    assert(match_regex("192\\.168\\.*", "192.168.1.250") == 1);
    assert(match_regex("192\\.168\\.*", "192.164.1.250") == 0);
    assert(match_regex("^2.3.*$", "2.3.4.6") == 1);
}

void test_filter() {
    int count = init_filter("192.168.1.*,192.168.223.*,100.100.23.*,2.3.*");
    assert(count == 4);
    assert(filter_by_addr("192.168.1.1") == 1);
    assert(filter_by_addr("192.168.1.256") == 1);
    assert(filter_by_addr("192.168.2.256") == 0);
    assert(filter_by_addr("192.168.223.1") == 1);
    assert(filter_by_addr("192.168.223.256") == 1);
    assert(filter_by_addr("192.166.223.256") == 0);
    assert(filter_by_addr("2.3.4.6") == 1);
    assert(filter_by_addr("2.3.126.123") == 1);
    assert(filter_by_addr("1.3.126.123") == 0);
    destroy_filter();
    assert(filter_by_addr("192.168.1.1") == 0);
    assert(filter_by_addr("192.168.223.256") == 0);
    assert(filter_by_addr("2.3.126.123") == 0);

    count = init_filter("*");
    assert(count == 1);
    assert(filter_by_addr("192.168.1.1") == 1);
    destroy_filter();
    assert(filter_by_addr("192.168.1.1") == 0);
}

int main(int argc, char **argv) {
    test_regex_match();

    test_filter();
}