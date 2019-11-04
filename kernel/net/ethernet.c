#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <kernel/net/ethernet.h>

struct ethernet_packet *net_create_ethernet_packet(struct mac_address dest, struct mac_address source, uint16_t type, void *payload, size_t payload_size) {
    struct ethernet_packet *packet = malloc(sizeof(struct ethernet_packet) + payload_size);
    assert(packet);

    packet->mac_destination = dest;
    packet->mac_source = source;
    packet->ether_type = type;
    memcpy(packet->payload, payload, payload_size);

    return packet;
}