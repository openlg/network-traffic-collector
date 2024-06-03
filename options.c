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
    options.debug = 0;
    options.port = 8010;
    options.buffer_size = 1024;

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
"   -f filter           Specify the list of matching rules that need to be filtered out.\n"
"                       Multiple rules are separated by commas.\n"
"                       Supports * to match all characters.\n"
"                       e.g:\n"
"                           192.168.1.66                          only filter this ip packet.\n"
"                           192.168.1.66,172.128.2.*              can filter all packets of 192.168.1.66\n"
"                                                                 and 172.128.2 network segment\n"
"                           192.168.1.66,172.128.2.*,100.100.*    can filter all packets of 192.168.1.66\n"
"                                                                 and 172.128.2 network segment\n"
"                                                                 and 100.100 network segment\n"
"   -A accessKey        Specify the access key used for signing, default get from env 'accessKey'\n"
"   -S secretKey        Specify the secret key used for signing, default get from env 'secretKey'\n"
"   -d                  Output debug log, This will output all captured packet header information, use with caution\n"
"\n"
"ntc, version v1.1.0\n"
"copyright (c) 2024 Leon Li <leon@tapdata.io>\n"
    );
}

void options_read_args(int argc, char **argv) {
    int opt;

    opterr = 0;
    char opt_str[] = "hi:s:I:f:A:S:dp:";
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
            case 'd':
                options.debug = 1;
                break;
            case 'p':
                options.port = atoi(optarg);
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