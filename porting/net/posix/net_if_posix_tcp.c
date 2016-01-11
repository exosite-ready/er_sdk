/**
 * @file net_if_posix_tcp.c
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
 * @brief Net_dev_operations implementation for the Posix network stack for TCP
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lib/error.h>
#include <lib/debug.h>
#include <porting/net_port.h>

static struct net_dev_operations eth_ops;

static struct sockaddr_in getipa(const char *hostname, int port)
{
    struct sockaddr_in ipa;

    ipa.sin_family = AF_INET;
    ipa.sin_port = htons(port);

    struct hostent *localhost = gethostbyname(hostname);

    if (!localhost) {
        error_log(DEBUG_NET, ("Resolving localhost\n"), ERR_FAILURE);
        return ipa;
    }

    char *addr = localhost->h_addr_list[0];

    memcpy(&ipa.sin_addr.s_addr, addr, sizeof(ipa.sin_addr.s_addr));

    return ipa;
}

static int32_t net_if_socket_internal(struct net_socket **new_socket,
                                      enum transport_protocol_type type,
                                      const int *posix_socket)
{
    int sockfd = 0;
    struct net_socket *nsocket = NULL;
    int flags;

    if (posix_socket == NULL) {
        switch (type) {
        case TRANSPORT_PROTO_UDP:
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            break;
        case TRANSPORT_PROTO_TCP:
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            break;
        default:
            ASSERT(0);
            break;
        }

        if (sockfd < 0) {
            error_log(DEBUG_NET, ("Failed to create socket %d\n", sockfd), errno);
            return ERR_FAILURE;
        }

        flags = fcntl(sockfd, F_GETFL, 0);
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
            error_log(DEBUG_NET, ("Ioctl FAILED\n"), errno);
            return ERR_FAILURE;
        }
    } else {
        ASSERT(*posix_socket >= 0);
        sockfd = *posix_socket;
    }

    nsocket = malloc(sizeof(*nsocket));
    if (nsocket) {
        nsocket->sockfd = sockfd;
        nsocket->ops = &eth_ops;
        nsocket->rflags = 0;
        nsocket->wflags = 0;
        nsocket->dtls = 0;
        nsocket->non_blocking = 1;
        nsocket->connected = 0;
        nsocket->already_started = 0;
    } else {
        error_log(DEBUG_NET, ("Failed to allocate memory for socket\n"), ERR_NO_MEMORY);
        return ERR_NO_MEMORY;
    }

    *new_socket = nsocket;
    return ERR_SUCCESS;
}

static int32_t net_if_socket_tcp(struct net_dev_operations *self, struct net_socket **socket)
{
    (void)self;
    return net_if_socket_internal(socket, TRANSPORT_PROTO_TCP, NULL);
}

static int32_t net_if_server_socket_tcp(struct net_dev_operations *self,
                                        struct net_socket **server_socket,
                                        const char *ip,
                                        unsigned short port)
{
    int status = ERR_SUCCESS;
    struct sockaddr_in isa = getipa("localhost", port);

    (void)self;
    (void)ip;

    /** 1. Create socket */
    status = net_if_socket_internal(server_socket, TRANSPORT_PROTO_TCP, NULL);
    if (status != ERR_SUCCESS)
        return status;

    /** 2. Bind socket to an address */
    status = bind((*server_socket)->sockfd, (struct sockaddr *)&isa, sizeof(isa));
    if (status != 0) {
        error_log(DEBUG_NET, ("binding socket to IP address\n"), status);
        close((*server_socket)->sockfd);
        return ERR_FAILURE;
    }

    /** 3. Start listening on the socket */
    status = listen((*server_socket)->sockfd, 1);
    if (status != 0) {
        error_log(DEBUG_NET, ("setting socket to listen\n"), status);
        close((*server_socket)->sockfd);
        return ERR_FAILURE;
    }

    return ERR_SUCCESS;
}

static int32_t net_if_socket_udp(struct net_dev_operations *self, struct net_socket **socket)
{
    (void)self;
    return net_if_socket_internal(socket, TRANSPORT_PROTO_UDP, NULL);
}

static int32_t net_if_connect(struct net_socket *socket, const char *ip,
        unsigned short port)
{
    struct sockaddr_in addr;
    int error;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (socket->connected)
        return ERR_SUCCESS;

    error = connect(socket->sockfd, (const struct sockaddr *) &addr,
            sizeof(addr));

    if (error)
        switch (errno) {
        case EINPROGRESS:
        case EALREADY:
            return ERR_WOULD_BLOCK;
        case EISCONN:
            socket->connected = TRUE;
            return ERR_SUCCESS;
        default:
            error_log(DEBUG_NET, ("Connect failed\n"), error);
            return ERR_FAILURE;
        }

    socket->connected = TRUE;
    return error;
}

static int32_t net_if_accept(struct net_socket *server_socket, struct net_socket **socket)
{
    int status = ERR_SUCCESS;
    int posix_socket;

    /** 1. Accept incoming connection request */
    posix_socket = accept(server_socket->sockfd, NULL, NULL);
    if (posix_socket == -1) {

        if (errno == EWOULDBLOCK || errno == ERR_WOULD_BLOCK)
            return ERR_WOULD_BLOCK;

        error_log(DEBUG_NET, ("accepting a connection\n"), ERR_FAILURE);
        return ERR_FAILURE;
    }

    /** 2. Create a new socket for the client */
    status = net_if_socket_internal(socket, TRANSPORT_PROTO_TCP, &posix_socket);
    if (status != ERR_SUCCESS)
        return status;

    return ERR_SUCCESS;
}

static int net_if_receive(struct net_socket *net_if, char *buf, size_t size, size_t *nbytes_recvd)
{
    int recvd;
    int sd = net_if->sockfd;

    recvd = (int) recv(sd, buf, size, net_if->rflags);

    if (recvd < 0) {
        int err;

        err = errno;
        if (err == EWOULDBLOCK || err == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return recvd;
    }

    if (recvd == 0) {
        error_log(DEBUG_NET, ("Remote host closed the connection\n"), ERR_FAILURE);
        return ERR_FAILURE;
    }

    *nbytes_recvd = (size_t)recvd;
    return ERR_SUCCESS;
}

static int net_if_send(struct net_socket *net_if, const char *buf, size_t size, size_t *nbytes_sent)
{
    int sd = net_if->sockfd;
    int sent;
    size_t len = size;

    sent = (int) send(sd, &buf[size - len], len, net_if->wflags);

    if (sent < 0) {
        int err;

        err = errno;
        if (err == EWOULDBLOCK || err == EAGAIN)
            if (net_if->non_blocking)
                return ERR_WOULD_BLOCK;

        return sent;
    }

    *nbytes_sent = (size_t)sent;
    return ERR_SUCCESS;
}

static int net_if_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    struct addrinfo hints, *res;
    struct in_addr addr;
    int err;

    (void)self;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    err = getaddrinfo(hostname, NULL, &hints, &res);
    if (err != 0) {
        error_log(DEBUG_NET, ("%s\n", gai_strerror(err)), err);
        error_log(DEBUG_NET, ("DNS error while resolving %s\n", hostname), err);
        return ERR_NET_DNS_ERROR;
    }

    addr.s_addr = ((struct sockaddr_in *)(void *)(res->ai_addr))->sin_addr.s_addr;
    inet_ntop(AF_INET, &addr, ipaddr_str, len);
    debug_log(DEBUG_NET, ("ip address : %s\n", ipaddr_str));

    freeaddrinfo(res);
    return ERR_SUCCESS;
}

static int net_if_close(struct net_socket *net_if)
{
    close(net_if->sockfd);
    free(net_if);
    return ERR_SUCCESS;
}

static struct net_dev_operations tcp_ops = {
        net_if_socket_tcp,          /* socket */
        net_if_server_socket_tcp,   /* server_socket */
        net_if_connect,             /* connect */
        net_if_accept,              /* accept */
        net_if_close,               /* close */
        net_if_receive,             /* recv */
        net_if_send,                /* send */
        net_if_lookup,              /* lookup */
        NULL,                       /* config */
        NULL,                       /* set_identity */
        NULL                        /* deinit */
};

struct net_dev_operations udp_ops = {
        net_if_socket_udp,          /* socket */
        NULL,                       /* server_socket */
        net_if_connect,             /* connect */
        NULL,                       /* accept */
        net_if_close,               /* close */
        net_if_receive,             /* recv */
        net_if_send,                /* send */
        net_if_lookup,              /* lookup */
        NULL,                       /* config */
        NULL,                       /* set_identity */
        NULL                        /* deinit */
};

struct net_dev_operations *net_dev_tcp_new()
{
    return &tcp_ops;
}

