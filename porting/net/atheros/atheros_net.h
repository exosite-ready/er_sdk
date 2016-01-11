#ifndef _ATHEROS_NET_H_
#define _ATHEROS_NET_H_

typedef union {
    uint32_t ipaddr;
    uint8_t ipbytes[4];
    uint16_t ipwords[2];
} atheros_net_ip;

#endif
