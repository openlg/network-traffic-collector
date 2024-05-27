//
// Created by Leon on 2024/5/24.
//
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

void packet_handler(u_char *user, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    printf("Packet capture length: %d\n", pkthdr->caplen);
    printf("Packet total length: %d\n", pkthdr->len);

    // 以太网头部
    struct ether_header *eth_header = (struct ether_header *) packet;
    printf("Ethernet source: %s\n", ether_ntoa((struct ether_addr *)eth_header->ether_shost));
    printf("Ethernet destination: %s\n", ether_ntoa((struct ether_addr *)eth_header->ether_dhost));
    printf("Ethernet type: %x\n", ntohs(eth_header->ether_type));

    if (ntohs(eth_header->ether_type) == ETHERTYPE_IP) {
        // IP 头部
        const struct ip *ip_header = (struct ip *)(packet + sizeof(struct ether_header));
        printf("IP source: %s\n", inet_ntoa(ip_header->ip_src));
        printf("IP destination: %s\n", inet_ntoa(ip_header->ip_dst));
    }

    printf("\n");
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *device;
    pcap_t *handle;

    // 获取可用的设备列表
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error finding devices: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    // 打印可用的设备列表
    printf("Available devices:\n");
    for (device = alldevs; device != NULL; device = device->next) {
        printf("%s - %s\n", device->name, (device->description) ? device->description : "No description available");
    }

    // 选择第一个设备
    device = alldevs;
    if (device == NULL) {
        fprintf(stderr, "No devices found.\n");
        exit(EXIT_FAILURE);
    }

    // 打开设备进行捕获
    handle = pcap_open_live(device->name, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Error opening device %s: %s\n", device->name, errbuf);
        pcap_freealldevs(alldevs);
        exit(EXIT_FAILURE);
    }

    // 捕获数据包
    pcap_loop(handle, 10, packet_handler, NULL);

    // 关闭句柄
    pcap_close(handle);
    pcap_freealldevs(alldevs);

    return 0;
}
