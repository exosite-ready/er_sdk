#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/error.h>

#include <porting/net_port.h>
#include <porting/system/exos/platform_exos.h>

#include <system/reset/sys_reset.h>
#include "system/ports/sys_ports.h"
#include <tcpip/tcpip.h>
#include <driver/wifi/mrf24w/drv_wifi.h>



static struct net_dev_operations eth_ops;

static int32_t net_if_socket(struct net_dev_operations *self, struct net_socket **new_socket)
{
    struct net_socket *socket = NULL;
    int32_t error;

    (void)self;
    debug_log(DEBUG_NET, ("Create WCM TCP socket\n"));
    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;
    if (socket) {
        socket->ops = &eth_ops;
        socket->non_blocking = 1;
        socket->dtls = 0;
        socket->connected = 0;
        socket->already_started = 0;
        socket->sockfd = INVALID_SOCKET;
    }

    *new_socket = socket;
    return ERR_SUCCESS;
}

/*
 * The current implementation will listen on any IP regardless of the IP provided
 **/
static int32_t net_if_server_socket(struct net_dev_operations *self,
                                    struct net_socket **new_socket,
                                    const char *ip,
                                    unsigned short port)
{
    struct net_socket *socket = NULL;
    int32_t error;

    (void)self;
    (void)ip;
    (void)port;

    debug_log(DEBUG_NET, ("Create WCM TCP socket\n"));
    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;
    if (socket) {
        socket->ops = &eth_ops;
        socket->non_blocking = 1;
        socket->dtls = 0;
        socket->connected = 0;
        socket->already_started = 0;
        socket->sockfd = INVALID_SOCKET;
        if (port != 0)
            socket->port = port;
    }

    *new_socket = socket;
    return ERR_SUCCESS;
}

static int32_t net_if_connect(struct net_socket *socket, const char *ip, uint16_t port)
{
    TCP_SOCKET tcpsock = (TCP_SOCKET) socket->sockfd;

    if (socket->connected)
        return ERR_SUCCESS;

    if (!socket->already_started) {
        IPV4_ADDR ip_num;

        TCPIP_Helper_StringToIPAddress(ip, &ip_num);

        /* Open a socket to the remote server */
        debug_log(DEBUG_NET, ("Connecting WCM TCP socket on port %s %d\n", ip, port));
        tcpsock = TCPIP_TCP_ClientOpen(IP_ADDRESS_TYPE_IPV4, port, (IP_MULTI_ADDRESS *) &ip_num);
        if (tcpsock == INVALID_SOCKET)
            return ERR_FAILURE;

        TCPIP_TCP_WasReset(tcpsock);
        socket->sockfd = (int32_t)tcpsock;
        socket->already_started = TRUE;
    }
    socket->connected = TCPIP_TCP_IsConnected(tcpsock);
    if (socket->connected) {
        BSP_LEDOn(BSP_LED_6);
        return ERR_SUCCESS;
    } else {
        return ERR_WOULD_BLOCK;
    }
}

int32_t net_if_accept(struct net_socket *socket, struct net_socket **new_socket)
{
    int32_t error;

    if (!socket->already_started) {
        TCP_SOCKET tcpsock;

        /* Currently we listen on any IP regardless of the IP provided */
        debug_log(DEBUG_NET, ("Listening on IP: any; port: %d\n", socket->port));
        tcpsock = TCPIP_TCP_ServerOpen(IP_ADDRESS_TYPE_ANY, socket->port, NULL);
        if (tcpsock == INVALID_SOCKET)
            return ERR_FAILURE;

        TCPIP_TCP_WasReset(tcpsock);
        socket->sockfd = (int32_t)tcpsock;
        socket->already_started = TRUE;
    } else {
        TCP_SOCKET tcpsock = (TCP_SOCKET) socket->sockfd;
        struct net_socket *server_socket;

        socket->connected = TCPIP_TCP_IsConnected(tcpsock);
        if (socket->connected) {
            error = socket->ops->server_socket(socket->ops, new_socket, "", socket->port);
            if (error != ERR_SUCCESS)
                return error;

            server_socket = *new_socket;
            server_socket->sockfd = socket->sockfd;
            server_socket->connected = TRUE;
            socket->already_started = FALSE;
            socket->connected = FALSE;
            BSP_LEDOn(BSP_LED_6);
            debug_log(DEBUG_NET, ("WCM TCP server socket connected\n"));
            return ERR_SUCCESS;
        }
    }

    return ERR_WOULD_BLOCK;
}

static int32_t net_if_receive(struct net_socket *socket, char *buf, size_t size, size_t *nbytes_received)
{
    uint16_t buffer_size;
    TCP_SOCKET sock = (TCP_SOCKET) socket->sockfd;

    if (TCPIP_TCP_WasReset(sock)) {
        debug_log(DEBUG_NET, ("Netif send: socket was reset\n"));
        return ERR_FAILURE;
    }
    buffer_size = TCPIP_TCP_GetIsReady(sock);
    if (buffer_size == 0)
        return ERR_WOULD_BLOCK;

    buffer_size = TCPIP_TCP_ArrayGet(sock, (uint8_t *) buf, size);

    *nbytes_received = buffer_size;
    return ERR_SUCCESS;
}

static int32_t net_if_close(struct net_socket *socket)
{
    TCP_SOCKET sock = (TCP_SOCKET) socket->sockfd;

    TCPIP_TCP_Close(sock);
    sf_free(socket);
    return 0;
}

static int32_t net_if_send(struct net_socket *socket, const char *buf, size_t size, size_t *nbytes_sent)
{
    register TCP_SOCKET sock = socket->sockfd;
    uint16_t buffer_size;

    if (TCPIP_TCP_WasReset(sock)) {
        debug_log(DEBUG_NET, ("Netif send: socket was reset\n"));
        return ERR_FAILURE;
    }
    debug_log(DEBUG_NET, ("WCM TCP sending data (%d)\n", size));
    buffer_size = TCPIP_TCP_PutIsReady(sock);
    if (buffer_size == 0) {
        debug_log(DEBUG_NET, ("WCM TCP buffer is not ready cannot send (%d)\n", size));
        return ERR_WOULD_BLOCK;
    }
    buffer_size = TCPIP_TCP_ArrayPut(sock, (uint8_t *) buf, (uint16_t) size);
    if (buffer_size < size)
        debug_log(DEBUG_NET, ("WCM TCP Could not send all %d/%d\n", buffer_size, size));

    *nbytes_sent = buffer_size;
    return ERR_SUCCESS;
}

/* TODO put this into header; this function cannot be statice net_if_wcm_udp.c uses it */
int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len);

int32_t net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    TCPIP_DNS_RESULT result;
    IPV4_ADDR addr;

    (void)self;
    debug_log(DEBUG_PLATFORM, ("Resolving %s...\n", hostname));

    /*
     * TODO workaround: remove all DNS entries The DNS resolution
     * sometimes gets the address of the WIFI router as a response this
     * happens if: - there is no uplink in the wifi router - there was a
     * wifi disconnect/reconnect (in this case it does not always happen)
     **/
    TCPIP_DNS_RemoveAll();
    result = TCPIP_DNS_Resolve(hostname, DNS_TYPE_A);
    /* ||DNS_TYPE_AAAA); */

    if (result != DNS_RES_OK) {
        debug_log(DEBUG_PLATFORM, ("Error [%d] Starting DNS Resolve for: %s\r\n", result, hostname));
        debug_log(DEBUG_PLATFORM, ("Cannot recover, workaround: restarting device!!!\r\n"));
        /* TEMPORARY WORKAROUND !!! */
        /*
         * Sometimes the TCPIP_DNS_Resolve returns with "no memory" error.
         *  To avoid stucking the code we enforce a software reset.
         **/
        if (result == DNS_RES_MEMORY_FAIL) {
            DRV_WIFI_Disconnect();
            SYS_RESET_SoftwareReset();
        }
        return ERR_FAILURE;
    }
    while ((result = TCPIP_DNS_IsResolved(hostname, &addr)) == DNS_RES_PENDING)
        SYS_Tasks();

    if (result != DNS_RES_OK) {
        debug_log(DEBUG_PLATFORM, ("Error [%d] Resolving DNS: %s\r\n", result, hostname));
        return ERR_FAILURE;
    } else {
        TCPIP_Helper_IPAddressToString(&addr, ipaddr_str, len);
        debug_log(DEBUG_PLATFORM, ("Resolved \"%s\" as: %s\r\n", hostname, ipaddr_str));
        return ERR_SUCCESS;
    }
}

static int32_t net_if_config(struct mac_addr *mac)
{
    return 0;
}

struct net_dev_operations *net_dev_tcp_new()
{
    return &eth_ops;
}

static struct net_dev_operations eth_ops = {
    net_if_socket,
    net_if_server_socket,
    net_if_connect,
    net_if_accept,
    net_if_close,
    net_if_receive,
    net_if_send,
    net_if_lookup,
    net_if_config,
    NULL
};
