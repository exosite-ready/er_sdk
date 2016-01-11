/**
 * @file net_if_atheros_tcp.c
 *
 * @copyright
 * Please read the Exosite Copyright: @ref copyright
 *
 * @if Authors
 * @authors
 *   - Szilveszter Balogh (szilveszterbalogh@exosite.com)
 *   - Zoltan Ribi (zoltanribi@exosite.com)
 * @endif
 *
 * @brief Net_dev_operations implementation for the Atheros network stack for TCP
 **/
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <mqx.h>
#include <fio.h>
#include <enet.h>
#include <enet_wifi.h>
#include <athdefs.h>
#include "rtcs.h"
#include "ipcfg.h"
#include "atheros_wifi_api.h"
#include "atheros_wifi.h"
#include "atheros_stack_offload.h"

#include <ctype.h>
#include <errno.h>
#include <lib/type.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/error.h>

#include <porting/net_port.h>
#include "atheros_net.h"

static struct net_dev_operations eth_ops;

static int packip(const char *pip, int32 ip)
{
#if 0
    unsigned int i, len, temp[4];
    int ret;

    ret = sscanf(pip, "%u.%u.%u.%u%n", temp + 3, temp + 2, temp + 1, temp + 0, &len);

    printf("Ret %d, len %d\n", ret, len);
    if (ret != 4 /*|| len != strlen( pip )*/)
    return -1;

    for (i = 0; i < 4; i++) {
        if (temp[i] > 255)
        return -1;

        ip = (u8)temp[i];
    }
#endif
    return 0;
}

static int unpackip(atheros_net_ip ip, char *addr, int size)
{
    snprintf(addr, size, "%d.%d.%d.%d", (int)ip.ipbytes[0],
            (int)ip.ipbytes[1], (int)ip.ipbytes[2], (int)ip.ipbytes[3]);
    return 0;
}

static int_32 ath_inet_aton(const char *name,
/* [IN] dotted decimal IP address */
A_UINT32 *ipaddr_ptr
/* [OUT] binary IP address */
)
{
    boolean ipok = FALSE;
    uint_32 dots;
    uint_32 byte;
    _ip_address addr;

    addr = 0;
    dots = 0;
    for (;;) {

        if (!isdigit(*name))
            break;

        byte = 0;
        while (isdigit(*name)) {
            byte *= 10;
            byte += *name - '0';
            if (byte > 255)
                break;
            name++;
        }
        if (byte > 255)
            break;
        addr <<= 8;
        addr += byte;

        if (*name == '.') {
            dots++;
            if (dots > 3)
                break;
            name++;
            continue;
        }

        if ((*name == '\0') && (dots == 3))
            ipok = TRUE;

        break;

    }

    if (!ipok)
        return 0;

    if (ipaddr_ptr)
        *ipaddr_ptr = addr;

    return -1;

}

extern _enet_handle gehandle;

static int32_t net_if_socket(struct net_dev_operations *self, struct net_socket **new_socket)
{
    int32_t error;
    struct net_socket *socket;
    A_UINT32 socketHandle = 0;

    (void)self;
    socketHandle = t_socket((void *) gehandle, ATH_AF_INET, SOCK_STREAM_TYPE, 0);
    if (socketHandle == A_ERROR) {
        printf("GetERROR: Unable to create socket\n");
        return ERR_FAILURE;
    }

    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;

    if (socket) {
        socket->sockfd = (int) socketHandle;
        socket->ops = &eth_ops;
        socket->non_blocking = 1;
        socket->dtls = 0;
        socket->connected = 0;
    }

    *new_socket = socket;

    return ERR_SUCCESS;
}

static int net_if_connect(struct net_socket *socket, const char *ip,
        unsigned short port)
{
    int32 addr;
    SOCKADDR_T hostAddr;
    A_INT32 res;
    A_UINT32 socketHandle = (A_UINT32) socket->sockfd;
    int error;

    if (socket->connected)
        return 0;

    error = ath_inet_aton(ip, &addr);
    if (error != -1)
        return -1;
    /*if (error != ERR_SUCCESS)
     *{
     *error_log(DEBUG_NET, ("Netif connect Invalid address\n"));
     *printf("packing failed %s %x\n", ip, addr.ipaddr);
     *return -1;
     }
     */

    printf("Connecting TCP %s/%d %x\n", ip, port, addr);
    /* Connect to the HTTPS server */
    socket->addr = addr;
    socket->port = port;
    memset(&hostAddr, 0, sizeof(hostAddr));
    hostAddr.sin_addr = addr;
    hostAddr.sin_port = port;
    hostAddr.sin_family = ATH_AF_INET;
    res = t_connect((void *) gehandle, socketHandle, (&hostAddr),
            sizeof(hostAddr));
    if (res != A_OK) {
        printf("Connection failed %d\n", res);
        return -1;
    }

    socket->connected = TRUE;

    printf("socket connect suc\r\n");

    return res;
}

static int net_if_receive(struct net_socket *socket, char *buf, size_t sz, size_t *nbytes_received)
{
    A_UINT32 addr_len;
    SOCKADDR_T addr;
    A_UINT32 socketHandle = (A_UINT32) socket->sockfd;
    int32_t received;
    char *ptr;

    received = t_recv((void *) gehandle, socketHandle, (void **) &ptr, sz, 0);
    if (received < 0) {
        if (received == A_ERROR)
            return ERR_WOULD_BLOCK;
        else
            return ERR_FAILURE;
    }

	*nbytes_received = received;

	assert(*nbytes_received <= sz);

    memcpy(buf, ptr, received);
    zero_copy_free(ptr);

    return ERR_SUCCESS;
}

static int net_if_close(struct net_socket *socket)
{
    A_UINT32 socketHandle = (A_UINT32) socket->sockfd;

    t_shutdown(gehandle, socketHandle);
    sf_free(socket);
    return 0;
}

static int net_if_send(struct net_socket *socket, const char *buf, size_t sz, size_t *nbytes_sent)
{
    int error;
    A_UINT32 socketHandle = (A_UINT32) socket->sockfd;
    char *buffer;

    while ((buffer = CUSTOM_ALLOC(sz)) == NULL)
        ;
    memcpy(buffer, buf, sz);
    error = t_send((void *) gehandle, socketHandle, (unsigned char *) &buffer[0],
            sz, 0);
    if (buffer)
        CUSTOM_FREE(buffer);

    *nbytes_sent = error;
    return ERR_SUCCESS;
}

static int net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    DNC_CFG_CMD dnsCfg;
    DNC_RESP_INFO dnsRespInfo;

    (void)self;
    if (strlen(hostname) >= sizeof(dnsCfg.ahostname)) {
        printf("GetERROR: host name too long\n");
        return 1;
    }

	printf("hostname = %s\r\n", hostname);
    strcpy((char *)dnsCfg.ahostname, hostname);
    dnsCfg.domain = ATH_AF_INET;
    dnsCfg.mode = RESOLVEHOSTNAME;
    if (custom_ip_resolve_hostname(gehandle, &dnsCfg, &dnsRespInfo) != A_OK) {
        printf("GetERROR: Unable to resolve host name\r\n");
        return 1;
    }

    atheros_net_ip ip;

    ip.ipaddr = dnsRespInfo.ipaddrs_list[0];
    unpackip(ip, ipaddr_str, len);
    printf("Ipaddr %s\n", ipaddr_str);
    printf("Returning\n");
    return 0;
}

static int net_if_config(struct mac_addr *mac)
{
    return 0;
}

struct net_dev_operations *net_dev_tcp_new()
{
    return &eth_ops;
}

static struct net_dev_operations eth_ops = {
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
