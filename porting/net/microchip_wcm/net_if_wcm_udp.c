#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/error.h>

#include <porting/net_port.h>

#include <tcpip/tcpip.h>
#include <tcpip/udp.h>

/* #include <tcpip/src/udp_manager.h> */
extern uint8_t *TCPIP_UDP_TxPointerGet(UDP_SOCKET s);

static struct net_dev_operations udp_ops;


static int32_t net_if_socket(struct net_dev_operations *self, struct net_socket **new_socket)
{
    struct net_socket *socket = NULL;
    int32_t error;

    (void)self;
    debug_log(DEBUG_NET, ("Create UDP socket\n"));
    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;

    if (socket) {
        socket->ops = &udp_ops;
        socket->non_blocking = 1;
        socket->dtls = 0;
        socket->connected = 0;
        socket->already_started = 0;
        socket->sockfd = INVALID_UDP_SOCKET;
    }

    *new_socket = socket;
    return ERR_SUCCESS;
}

static int32_t net_if_connect(struct net_socket *socket, const char *ip, uint16_t port)
{
    UDP_SOCKET udpsocket;
    IPV4_ADDR ip_num;
    uint16_t buffer_size;

    TCPIP_Helper_StringToIPAddress(ip, &ip_num);

    if (socket->connected)
        return ERR_SUCCESS;

    udpsocket = TCPIP_UDP_ClientOpen(IP_ADDRESS_TYPE_IPV4, port, (IP_MULTI_ADDRESS *) &ip_num);
    buffer_size = TCPIP_UDP_TxPutIsReady(udpsocket, 0);
    debug_log(DEBUG_NET, ("UDP connect, buffer size: %d\n", buffer_size));
    TCPIP_UDP_OptionsSet(udpsocket, UDP_OPTION_STRICT_NET, (void *)true);
    TCPIP_UDP_OptionsSet(udpsocket, UDP_OPTION_STRICT_ADDRESS, (void *)true);
    socket->sockfd = (int32_t)udpsocket;
    socket->connected = TRUE;

    return ERR_SUCCESS;
}

static int32_t net_if_receive(struct net_socket *socket, char *buf, size_t size, size_t *nbytes_received)
{
    uint16_t buffer_size;
    UDP_SOCKET udpsocket = (UDP_SOCKET) socket->sockfd;

    buffer_size = TCPIP_UDP_GetIsReady(udpsocket);
    if (buffer_size == 0)
        return ERR_WOULD_BLOCK;

    buffer_size = TCPIP_UDP_ArrayGet(udpsocket, (uint8_t *)buf, buffer_size);

    *nbytes_received = buffer_size;
    return ERR_SUCCESS;
}

static int32_t net_if_close(struct net_socket *socket)
{
    UDP_SOCKET udpsocket = (UDP_SOCKET) socket->sockfd;

    TCPIP_UDP_Close(udpsocket);
    return ERR_SUCCESS;
}

static int32_t net_if_send(struct net_socket *socket, const char *buf, size_t size, size_t *nbytes_sent)
{
    UDP_SOCKET udpsocket = (UDP_SOCKET) socket->sockfd;
    uint8_t *wptr;
    uint16_t buffer_size;

    buffer_size = TCPIP_UDP_TxPutIsReady(udpsocket, size);
    if (buffer_size < size)
        return ERR_NO_MEMORY;

    /* this will put the start pointer at the beginning of the TX buffer */
    TCPIP_UDP_TxOffsetSet(udpsocket, 0, false);

    /* Get the write pointer: */
    wptr = TCPIP_UDP_TxPointerGet(udpsocket);
    if (!wptr)
        return ERR_WOULD_BLOCK;

    memcpy(wptr, buf, size);

    TCPIP_UDP_TxOffsetSet(udpsocket, size, false);
    TCPIP_UDP_Flush(udpsocket);

    *nbytes_sent = size;

    return ERR_SUCCESS;
}

static int32_t net_if_config(struct mac_addr *mac)
{
    return 0;
}

struct net_dev_operations *net_dev_udp_new()
{
    return &udp_ops;
}

extern int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len);

static struct net_dev_operations udp_ops = {
    net_if_socket,
    NULL,
    net_if_connect,
    NULL,
    net_if_close,
    net_if_receive,
    net_if_send,
    net_if_lookup,
    net_if_config,
    NULL
};
