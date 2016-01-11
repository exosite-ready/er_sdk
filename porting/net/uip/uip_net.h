/* Network services */

#ifndef __UIP_NET_H__
#define __UIP_NET_H__

/* These types are only used by the UIP and UIP wrappers*/
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef unsigned long long timer_data_type;

/* network typedefs */
typedef s16 uip_net_size;

/* IP address type */
typedef union {
    u32 ipaddr;
    u8 ipbytes[4];
    u16 ipwords[2];
} uip_net_ip;

/* services ports */
#define UIP_NET_TELNET_PORT          23

/* Different constants */
#define UIP_NET_SOCK_STREAM          0
#define UIP_NET_SOCK_DGRAM           1

/* 'no lastchar' for read to char (recv) */
#define UIP_NET_NO_LASTCHAR          ( -1 )

/* TCP/IP functions */
int uip_net_socket(int type, int non_blocking);
int uip_net_close(int s);
/* uip_net_size uip_net_recvbuf( int s, luaL_Buffer *buf, uip_net_size maxsize, s16 readto, unsigned timer_id, timer_data_type to_us ); */
uip_net_size uip_net_recv(int s, void *buf, uip_net_size maxsize, s16 readto,
        unsigned timer_id, timer_data_type to_us);
uip_net_size uip_net_send(int s, const void* buf, uip_net_size len);
int uip_accept(u16 port, unsigned timer_id, timer_data_type to_us,
        uip_net_ip* pfrom);
int uip_net_connect(int s, uip_net_ip addr, u16 port);
int uip_net_lookup(const char* hostname, uip_net_ip* res);

int uip_net_get_last_err(int s);
int uip_net_get_telnet_socket(void);

#endif
