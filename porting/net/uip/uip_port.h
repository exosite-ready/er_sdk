/*
 * UIP "helper"
 * Implements the UIP application
 */

#ifndef __UIP_PORT_H__
#define __UIP_PORT_H__

#include "uip_net.h"

/* UIP application states */
enum {
    UIP_STATE_IDLE = 0,
    UIP_STATE_SEND,
    UIP_STATE_RECV,
    UIP_STATE_RECV_2,
    UIP_STATE_CONNECT,
    UIP_STATE_CLOSE,
    UIP_STATE_TOIDLE
};

/* UIP state */
struct uip_state {
    u8 state;
    int res;
    char* ptr;
    uip_net_size len;
    s16 readto;
    int inuse;
    struct tcp_buffer_class *tcp_inbuf;
    char *inbuf;
    char *inptr;
    uip_net_size inbuf_len;
    uip_net_size inbuf_max_len;
    char *outbuf;
    char *outptr;
    uip_net_size outbuf_len;
    uip_net_size outbuf_max_len;
    int with_buffer;
    int non_blocking;
    int connect_started;
    int connected;
};

struct uip_eth_addr;

/* Helper functions */
void uip_appcall(void);
void uip_udp_appcall(void);
void udp_socket_receive_appcall(void);
void uip_get_eth_addr(struct uip_eth_addr *paddr);
void uip_mainloop(void* dummy);

#endif /* __UIP_PORT_H__ */
