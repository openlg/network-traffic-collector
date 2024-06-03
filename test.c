//
// Created by Leon on 2024/5/29.
//
#include <stdio.h>
#include <assert.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include "filter.h"
#include "signature.h"


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

void test_hmac_sha1() {
    char text[] = "POST:accessKey=IBkjcVhwHQfKUX9aezvr5bFmgVKZSxfX&nonce=umo2Fjc81i&signVersion=1.0&ts=1717140010000:{\"metadata\":{\"interface\": \"en0\", \"hostname\": \"leon-pc.local\", \"process_id\": \"\", \"version\": \"\", \"mac\": \"14:7d:da:2a:e8:c1\", \"ip\": \"192.168.1.66\", \"ipv6\": \"::\" },\"transmit\": 43964.000000, \"received\": 50535.000000, \"start\": 1717139994, \"end\": 1717140010 }";
    char key[] = "yWErSN7alUa8KhjXIWoPxcNqLubStQlp";
    char result[] = "dHn/LdvrM1p4H05aAO9fEGdL3ew=";
    unsigned int sha1_len = 0;
    unsigned char *sha1 = compute_hmac_sha1(key, text, &sha1_len);

    for (int i = 0; i < sha1_len; ++i) {
        printf("%hx", sha1[i]);
    }

    char *base64 = base64_encode(sha1, sha1_len);
    printf("\n%s\n", base64);
    assert(strcmp(result, result) == 0);

    free(sha1);
}

int main(int argc, char **argv) {
    test_regex_match();
    test_filter();
    //test_hmac_sha1();
}