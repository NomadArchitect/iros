#ifndef _KERNEL_NET_ARP_H
#define _KERNEL_NET_ARP_H 1

#include <stdint.h>

#include <kernel/net/ip_address.h>
#include <kernel/net/mac.h>

#define ARP_PROTOCOL_TYPE_ETHERNET 1
#define ARP_PROTOCOL_TYPE_IP_V4    0x0800

#define ARP_OPERATION_REQUEST 1
#define ARP_OPERATION_REPLY   2

struct arp_packet {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_addr_len;
    uint8_t protocol_addr_len;
    uint16_t operation;
    struct mac_address mac_sender;
    struct ip_v4_address ip_sender;
    struct mac_address mac_target;
    struct ip_v4_address ip_target;
} __attribute__((packed));

struct arp_packet *net_create_arp_packet(uint16_t op, struct mac_address s_mac, struct ip_v4_address s_ip, struct mac_address t_mac, struct ip_v4_address t_ip);

#endif /* _KERNEL_NET_ARP_H */