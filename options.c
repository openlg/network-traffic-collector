//
// Created by Leon on 2024/5/23.
//

#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <printf.h>
#include <stdlib.h>
#include <stdio.h>

#include "options.h"
#include "net/if.h"
#include "ntc.h"

options_t options;

static char *bad_interface_names[] = {
        "lo:",
        "lo",
        "stf",     /* pseudo-device 6to4 tunnel interface */
        "gif",     /* psuedo-device generic tunnel interface */
        "dummy",
        "vmnet",
        "wmaster", /* wmaster0 is an internal-use interface for mac80211, a Linux WiFi API. */
        NULL       /* last entry must be NULL */
};

static int is_bad_interface_name(char *i) {
    char **p;
    for (p = bad_interface_names; *p; ++p)
        if (strncmp(i, *p, strlen(*p)) == 0)
            return 1;
    return 0;
}

static char *get_first_interface() {
    struct if_nameindex * name_index;
    struct ifreq ifr;
    char *i = NULL;
    int j = 0;
    int s;
    /* Use if_nameindex(3) instead? */

    name_index = if_nameindex();
    if(name_index == NULL) {
        return NULL;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0); /* any sort of IP socket will do */
    while(name_index[j].if_index != 0) {
        if (strcmp(name_index[j].if_name, "lo") != 0 && !is_bad_interface_name(name_index[j].if_name)) {
            strncpy(ifr.ifr_name, name_index[j].if_name, sizeof(ifr.ifr_name));
            if ((s == -1) || (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) || (ifr.ifr_flags & IFF_UP)) {
                i = xstrdup(name_index[j].if_name);
                break;
            }
        }
        j++;
    }
    if_freenameindex(name_index);
    return i;
}

void options_set_defaults() {
    options.interface = get_first_interface();
    if (!options.interface)
        options.interface = "eth0";
    if (options.interval == 0)
        options.interval = 60;
    if (options.url == NULL)
        options.url = "http://127.0.0.1:3000/health";
    if (options.accessKey == NULL)
        options.accessKey = getenv("accessKey");
    if (options.secretKey == NULL)
        options.secretKey = getenv("secretKey");

#ifdef NEED_PROMISCUOUS_FOR_OUTGOING
    options.promiscuous = 1;
    options.promiscuous_but_choosy = 1;
#else
    options.promiscuous = 0;
    options.promiscuous_but_choosy = 0;
#endif

}

void usage(FILE *fp) {
    fprintf(fp,
"ntc: network traffic collector usage on an interface by host\n"
"\n"
"Synopsis: ntc -h | [-i interface] [-u push data url]\n"
"\n"
"   -h                  display this message\n"
"   -i interface        listen on named interface\n"
"   -s server url       the collected network traffic information is regularly pushed to the server\n"
"   -I 1m               push data every minute, format is [1h][1m][20s], default is 1m\n"
"   -f filter           Specify a list of IP addresses that need to be filtered out. Use commas to \n"
"                       separate multiple IP addresses, such as '192.168.1.66,34.150.58.185'\n"
"   -A accessKey        Specify the access key used for signing, default get from env 'accessKey'\n"
"   -S secretKey        Specify the secret key used for signing, default get from env 'secretKey'\n"
"\n"
"ntc, version v1.0\n"
"copyright (c) 2024 Leon Li <leon@tapdata.io>\n"
    );
}

void options_read_args(int argc, char **argv) {
    int opt;

    opterr = 0;
    char opt_str[] = "hi:s:I:f:A:S:";
    while ((opt = getopt(argc, argv, opt_str)) != -1) {
        switch (opt) {
            case 'h':
                usage(stdout);
                exit(EXIT_SUCCESS);
            case 'i':
                options.interface = optarg;
                break;
            case 's':
                options.url = optarg;
                break;
            case 'I':
                options.interval = parse_time(optarg);
                break;
            case 'f':
                options.filter = optarg;
                break;
            case 'A':
                options.accessKey = optarg;
                break;
            case 'S':
                options.secretKey = optarg;
                break;
            case '?':
                fprintf(stderr, "ntc: unknown option -%c\n", optopt);
                usage(stderr);
                exit(EXIT_FAILURE);
            case ':':
                fprintf(stderr, "ntc: option -%c requires an argument\n", optopt);
                usage(stderr);
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "ntc: unknown option -%c\n", optopt);
                usage(stderr);
                exit(EXIT_FAILURE);
        }
    }

    if (optind != argc) {
        fprintf(stderr, "ntc: found arguments following options\n");
        usage(stderr);
        exit(EXIT_FAILURE);
    }
}

int options_check() {

    return 0;
}