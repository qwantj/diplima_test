#pragma once

#include <cstdint>

#pragma pack(push, 1)

struct eth_hdr {
    uint8_t  dmac[6];
    uint8_t  smac[6];
    uint16_t ethertype;
};

struct ipv4_hdr {
    uint8_t  ihl:4;
    uint8_t  version:4;
    uint8_t  tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};

struct ipv6_hdr {
    uint32_t vtc_flow; // 4 bits version, 8 bits TC, 20 bits flow-ID
    uint16_t payload_len;
    uint8_t  nexthdr;
    uint8_t  hop_limit;
    uint8_t  saddr[16];
    uint8_t  daddr[16];
};

struct tcp_hdr {
    uint16_t source;
    uint16_t dest;
    uint32_t seq;
    uint32_t ack_seq;
    uint16_t res1:4;
    uint16_t doff:4;
    uint16_t fin:1;
    uint16_t syn:1;
    uint16_t rst:1;
    uint16_t psh:1;
    uint16_t ack:1;
    uint16_t urg:1;
    uint16_t res2:2;
    uint16_t window;
    uint16_t check;
    uint16_t urg_ptr;
};

struct udp_hdr {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
};

#pragma pack(pop)
