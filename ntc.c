//
// Network traffic collection
// Created by lg on 5/22/24.
//

#include <stdio.h>
#include <pcap.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <locale.h>
#include <sys/signal.h>
#include <signal.h>
#include <pthread.h>
#include <curl/curl.h>
#include <unistd.h>
#include <stdlib.h>
#include "options.h"
#include "ntc.h"
#include "filter.h"
#include "log.h"
#include "signature.h"
#include "server.h"

/* ethernet address of interface. */
int have_hw_addr = 0;
unsigned char if_hw_addr[6];

/* IP address of interface */
int have_ip_addr = 0;
int have_ip6_addr = 0;
struct in_addr if_ip_addr;
struct in6_addr if_ip6_addr;

extern options_t options;
pthread_mutex_t tick_mutex;
volatile ThreadStatus thread_status;

pcap_t* pd; /* pcap descriptor */
struct bpf_program pcap_filter;

sig_atomic_t sigAtomic;

Metrics metrics;

#define CAPTURE_LENGTH 256

void ntc_init() {
    curl_global_init( CURL_GLOBAL_ALL );
    memset(&metrics, 0, sizeof metrics);

    int count = init_filter(options.filter);
    log_info("Compiled %d regular expressions to filter packet.", count);
}

int packet_init() {
    char error_buf[PCAP_ERRBUF_SIZE];
    int result;

    result = get_addrs_ioctl(options.interface, if_hw_addr,
                             &if_ip_addr, &if_ip6_addr);

    if (result < 0) {
        return 0;
    }
    have_hw_addr = result & 0x01;
    have_ip_addr = result & 0x02;
    have_ip6_addr = result & 0x04;

    if(have_ip_addr) {
        log_info("IP address is: %s", inet_ntoa(if_ip_addr));
    }
    if(have_ip6_addr) {
        char ip6str[INET6_ADDRSTRLEN];
        ip6str[0] = '\0';
        inet_ntop(AF_INET6, &if_ip6_addr, ip6str, sizeof(ip6str));
        log_info("IPv6 address is: %s", ip6str);
    }

    if(have_hw_addr) {
        char mac[3 * sizeof(if_hw_addr) + 1];
        for (int i = 0; i < sizeof(if_hw_addr); ++i) {
            sprintf(mac + 3 * i, "%c%02x", i ? ':' : ' ', (unsigned int)if_hw_addr[i]);
        }
        log_info("MAC address is: %s", mac);
    }

    pd = pcap_open_live(options.interface, CAPTURE_LENGTH, options.promiscuous, 1000, error_buf);

    if(pd == NULL) {
        log_error("pcap_open_live(%s): %s", options.interface, error_buf);
        return 0;
    }
    return 1;
}

void stop_packet() {
    if (pd != NULL) {
        pcap_close(pd);
    }
    log_info("Packet capture stopped.");
}

void ether_addr_to_string(struct ether_addr *addr, char *str) {
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            addr->ether_addr_octet[0],
            addr->ether_addr_octet[1],
            addr->ether_addr_octet[2],
            addr->ether_addr_octet[3],
            addr->ether_addr_octet[4],
            addr->ether_addr_octet[5]);
}

static void handle_packet(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char *packet) {
    int ether_type;

    char src_ip[INET6_ADDRSTRLEN];
    char dst_ip[INET6_ADDRSTRLEN];

    struct ether_header *eth_header = (struct ether_header *) packet;
    ether_type = ntohs(eth_header->ether_type);

    int dir = -1;
    if (ether_type == ETHERTYPE_IP || ether_type == ETHERTYPE_IPV6) {
        if(have_hw_addr && memcmp(eth_header->ether_shost, if_hw_addr, 6) == 0 ) {
            /* packet leaving this i/f */
            dir = 1;
        } else if (have_hw_addr && memcmp(eth_header->ether_dhost, if_hw_addr, 6) == 0 ) {
            /* packet entering this i/f */
            dir = 0;
        } else if (memcmp("\xFF\xFF\xFF\xFF\xFF\xFF", eth_header->ether_dhost, 6) == 0) {
            /* broadcast packet, count as incoming */
            dir = 0;
        }
    } else {
        return; // ignore other ether type packet
    }

    if (ether_type == ETHERTYPE_IP) {
        const struct ip *ip_header = (struct ip *)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET, &(ip_header->ip_src), src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip_header->ip_dst), dst_ip, INET_ADDRSTRLEN);
    } else {
        const struct ip6_hdr *ip6_header = (struct ip6_hdr *)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET6, &(ip6_header->ip6_src), src_ip, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(ip6_header->ip6_dst), dst_ip, INET6_ADDRSTRLEN);

        if( IN6_IS_ADDR_LINKLOCAL(&ip6_header->ip6_dst)
                || IN6_IS_ADDR_LINKLOCAL(&ip6_header->ip6_src) ) {
            return;
        }
    }

    if (options.debug) {
        char src_ether[40];
        char dst_ether[40];
        ether_addr_to_string((struct ether_addr *)eth_header->ether_shost, src_ether);
        ether_addr_to_string((struct ether_addr *)eth_header->ether_dhost, dst_ether);
        log_debug("%s(%s) => %s(%s)", src_ether, src_ip, dst_ether, dst_ip);
    }

    if (filter_by_addr(dir == 1 ? dst_ip : src_ip)) {
        if (options.debug) {
            log_debug("Filter matches the address %s and ignores the packet.\n", dir == 1 ? dst_ip : src_ip);
        }
        return;
    }
    pthread_mutex_lock(&tick_mutex);
    if (dir == 0) {
        metrics.total_recv += pkthdr->len;
    } else {
        metrics.total_sent += pkthdr->len;
    }
    metrics.ts = time(NULL);
    pthread_mutex_unlock(&tick_mutex);

    if (options.debug) {
        log_debug("%s => %s, dir: %d, type:%x, capture:%d, packet total:%d, total transmit: %Lf, total received: %Lf\n",
                  src_ip, dst_ip, dir, ether_type, pkthdr->caplen, pkthdr->len, metrics.total_sent, metrics.total_recv);
    }
}

/* packet_loop:
 * Worker function for packet capture thread. */
void packet_loop(void* ptr) {
    pcap_loop(pd,-1,(pcap_handler)handle_packet,NULL);
    thread_status = THREAD_FINISHED;
}

size_t read_response_data(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Response *response = (Response *)userp;

    char *ptr = realloc(response->data, response->size + realsize +1);
    if(ptr == NULL) {
        log_warn("not enough memory (realloc returned NULL)");
        return 0;
    }
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    return realsize;
}

size_t read_response_header(char *buffer, size_t size, size_t nitems, void *userdata) {
    struct ResponseHeaders *headers = (struct ResponseHeaders *)userdata;
    size_t length = size * nitems;

    if (strncasecmp(buffer, "HTTP", 4) == 0) {
        sscanf(buffer, "%s %d %s[^\n]", headers->http_version, &headers->status_code, headers->status_text);
    } else if(strncasecmp(buffer, "Content-Type:", 13) == 0) {
        headers->content_type = strdup(buffer + 13);
    } else if (strncasecmp(buffer, "Content-Length:", 15) == 0) {
        headers->content_length = atol(buffer + 15);
    }
    return length;
}

void push_data_loop() {

    CURL * curl = curl_easy_init ( ) ;

    if (curl) {
        CURLcode res;
        Response response;
        response.data = malloc(1);
        response.size = 0;
        char request_body[2048];

        char hostname[256];
        char *process_id = getenv("process_id");
        char *version = getenv("version");
        char mac[3 * sizeof(if_hw_addr) + 1];
        char ip6str[INET6_ADDRSTRLEN];
        struct curl_slist *headers = NULL;
        time_t start = time(NULL);

        int enable_sign = options.accessKey != NULL && options.secretKey != NULL;
        char *nonce = (char *)malloc(11 * sizeof(char));

        for (int i = 0; i < sizeof(if_hw_addr); ++i) {
            sprintf(mac + 3 * i, "%c%02x", i ? ':' : ' ', (unsigned int)if_hw_addr[i]);
        }
        memmove(mac, mac + 1, strlen(mac));

        ip6str[0] = '\0';
        inet_ntop(AF_INET6, &if_ip6_addr, ip6str, sizeof(ip6str));
        if (process_id == NULL)
            process_id = "";
        if (version == NULL)
            version = "";

        headers = curl_slist_append(headers, "Content-Type: application/json");

        char send_str[20];
        char recv_str[20];
        char datetime[80];
        time_t now;
        time_t end;
        double long send_bytes;
        double long recv_bytes;

        while (thread_status != THREAD_FINISHED && sigAtomic == 0) {
            sleep(options.interval);
            now = time(NULL);
            end = metrics.ts;
            send_bytes = metrics.total_sent;
            recv_bytes = metrics.total_recv;

            readable_size(send_bytes, send_str);
            readable_size(recv_bytes, recv_str);

            strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&now));

            if (gethostname(hostname, sizeof(hostname)) != 0) {
                strcpy(hostname, getenv("HOSTNAME"));
            }
            sprintf(request_body, "{"
                                  "\"metadata\":{"
                                      "\"interface\": \"%s\", "
                                      "\"hostname\": \"%s\", "
                                      "\"process_id\": \"%s\", "
                                      "\"version\": \"%s\", "
                                      "\"mac\": \"%s\", "
                                      "\"ip\": \"%s\", "
                                      "\"ipv6\": \"%s\" "
                                  "},"
                                  "\"transmit\": %Lf, "
                                  "\"received\": %Lf, "
                                  "\"start\": %ld, "
                                  "\"end\": %ld "
                                  "}",
                    options.interface, hostname, process_id, version, mac, inet_ntoa(if_ip_addr),
                    ip6str, send_bytes, recv_bytes, start, end);

            log_info("Total send %s, total receive %s, send data %s", send_str, recv_str, request_body);

            if (enable_sign) {
                generate_random_string(10, nonce);
                char ts[14];
                sprintf(ts, "%ld000", time(NULL));
                char *sign_str = sign(nonce, "1.0", options.accessKey, options.secretKey, ts, request_body);
				char *sign_str_ = curl_easy_escape(curl, sign_str, 0);
				char url[strlen(options.url) + strlen(options.accessKey) + strlen(sign_str_) + 100];

                sprintf(url, "%s?nonce=%s&ts=%s&accessKey=%s&sign=%s&signVersion=1.0",
                        options.url, nonce, ts, options.accessKey, sign_str_);
                curl_easy_setopt(curl, CURLOPT_URL, url);
                log_info("Send to %s", url);
				free(sign_str);
				free(sign_str_);
            } else {
                curl_easy_setopt(curl, CURLOPT_URL, options.url);
                log_info("Send to %s", options.url);
            }
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_response_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, read_response_header);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);

            res = curl_easy_perform( curl );
            if (res != CURLE_OK) {
                log_error("Send failed: %s", curl_easy_strerror(res));
            } else {
                if (response.headers.status_code >= 200
						&& response.headers.status_code < 300
						&& contains(response.data, "\"code\":\"ok\"")) {
                    log_info("Send successful: %d %s", response.headers.status_code, response.data);
                    pthread_mutex_lock(&tick_mutex);
                    metrics.total_sent -= send_bytes;
                    metrics.total_recv -= recv_bytes;
                    start = end;
                    pthread_mutex_unlock(&tick_mutex);
                } else {
                    log_error("Send failed: %d %s\n", response.headers.status_code, response.data);
                }
            }
            response.size = 0;
        }
        curl_easy_cleanup( curl );
    }
}

static void finish(int sig) {
    sigAtomic = sig;
    if (pd != NULL) {
        pcap_breakloop(pd);
    }
}

void ntc_destroy() {
    if (pd != NULL) {
        pcap_close(pd);
        log_info("Packet capture stopped.");
    }
    curl_global_cleanup();

    destroy_filter();
}

int main(int argc, char **argv) {

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    setlocale(LC_ALL, "");

    options_set_defaults();

    options_read_args(argc, argv);

    if ( options_check() != 0) {
        return 1;
    }

    struct sigaction sa = {};
    sa.sa_handler = finish;
    sigaction(SIGINT, &sa, NULL);

    ntc_init();

    pthread_t server_thread;
    int result = pthread_create(&server_thread, NULL, (void*)&start, NULL);
    if (result != 0) {
        log_error("Error creating server thread");
        return 1;
    }

    result = packet_init();

    if (result) {
        pthread_t thread;
        pthread_mutex_init(&tick_mutex, NULL);
        result = pthread_create(&thread, NULL, (void*)&packet_loop, NULL);
        if (result != 0) {
            log_error("Error creating thread");
        } else {
            thread_status = THREAD_RUNNING;
            push_data_loop();
        }
        pthread_cancel(thread);
        //pthread_join(thread, NULL);
    }

    log_info("Stopping ntc...");
    stop();
    sleep(1);
    pthread_cancel(server_thread);

    ntc_destroy();
    log_info("Ntc is stopped.");
	return 0;
}