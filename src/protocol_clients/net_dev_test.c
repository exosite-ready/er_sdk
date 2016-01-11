/**
 * @file net_dev_test.c
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
 * @brief
 * A @net_port_interface implementation that adds fault injection capabilities to the
 * underlying 'real' network interface
 **/
#ifdef TEST_ERSDK
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/fault_injection.h>
#include <lib/sf_malloc.h>
#include <lib/utils.h>
#include <porting/net_port.h>
#include <net_dev_test.h>

struct net_dev_test_operations {
    struct net_dev_operations ops;
    struct net_dev_operations *low_level_ops;
};

struct net_test_socket {
    struct net_socket self;
    struct net_socket *low_level_socket;
};

static enum fault_type recv_error = FT_NO_ERROR;
static enum fault_type send_error = FT_NO_ERROR;

static int32_t net_dev_socket(struct net_dev_operations *self, struct net_socket **s)
{
    int32_t error;
    struct net_socket *low_level_socket = NULL;
    struct net_dev_test_operations *dev;
    struct net_test_socket *socket;

    dev = container_of(self, struct net_dev_test_operations, ops);
    error = dev->low_level_ops->socket(dev->low_level_ops, &low_level_socket);

    error = sf_mem_alloc((void **)&socket, sizeof(*socket));
    if (error != ERR_SUCCESS)
        return error;

    socket->low_level_socket = low_level_socket;
    socket->self.ops = dev->low_level_ops;

    *s = &socket->self;

    return error;
}

static int32_t net_dev_connect(struct net_socket *socket, const char *ip,
        unsigned short port)
{
    int32_t error;
    struct net_test_socket *s;

    s = container_of(socket, struct net_test_socket, self);
    error = s->self.ops->connect(s->low_level_socket, ip, port);
    return error;
}

static int32_t recv_count;
static int32_t net_dev_receive(struct net_socket *socket, char *buf, size_t size, size_t *nbytes_recvd)
{
    int32_t error;
    struct net_test_socket *s;

    s = container_of(socket, struct net_test_socket, self);

    if (recv_error == FT_ERROR)
        return ERR_FAILURE;

    if (recv_error == FT_TIMEOUT_AFTER_INIT) {
        recv_count++;
        if (recv_count > 20)
            return ERR_WOULD_BLOCK;
    }


    if (recv_error == FT_TIMEOUT) {
        return ERR_WOULD_BLOCK;
    }

    if (recv_error == FT_ERROR_AFTER_INIT) {
        recv_count++;
        if (recv_count > 20)
            return ERR_FAILURE;
    }

    if (recv_error == FT_TEMPORARY_ERROR) {
        recv_count++;
        if (recv_count < 20)
            return ERR_FAILURE;
        else
            recv_count = 0;
    }

    error = s->self.ops->recv(s->low_level_socket, buf, size, nbytes_recvd);
    return error;
}

static int32_t send_count;
static int32_t net_dev_send(struct net_socket *socket, char *buf, size_t size, size_t *nbytes_sent)
{
    int32_t error;
    struct net_test_socket *s;

    s = container_of(socket, struct net_test_socket, self);

    if (send_error == FT_ERROR)
        return ERR_FAILURE;

    if (send_error == FT_ERROR_AFTER_INIT) {
        send_count++;
        if (send_count > 2)
            return ERR_FAILURE;
    }

    if (send_error == FT_TEMPORARY_ERROR) {
        send_count++;
        if (send_count < 20)
            return ERR_FAILURE;
        else
            send_count = 0;
    }

    error = s->self.ops->send(s->low_level_socket, buf, size, nbytes_sent);
    return error;
}

static int32_t net_dev_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t ip_size)
{
    int32_t error;
    struct net_dev_test_operations *dev_test;

    dev_test = container_of(self, struct net_dev_test_operations, ops);

    error = dev_test->low_level_ops->lookup(dev_test->low_level_ops, hostname, ipaddr_str, ip_size);
    return error;
}

static int32_t net_dev_close(struct net_socket *socket)
{
    int32_t error;
    struct net_test_socket *s;

    s = container_of(socket, struct net_test_socket, self);

    error = s->self.ops->close(s->low_level_socket);
    sf_mem_free(s);

    return error;
}


static struct net_dev_operations tcp_ops = {
        net_dev_socket,
        NULL,
        net_dev_connect,
        NULL,
        net_dev_close,
        net_dev_receive,
        net_dev_send,
        net_dev_lookup,
        NULL,
        NULL,
        NULL
};

struct net_dev_operations *net_dev_test_new(struct net_dev_operations *dev)
{
    int32_t error;
    struct net_dev_test_operations *net_dev_test;

    error = sf_mem_alloc((void **)&net_dev_test, sizeof(*net_dev_test));
    if (error != ERR_SUCCESS)
        return NULL;

    net_dev_test->ops = tcp_ops;
    net_dev_test->low_level_ops = dev;

    return &net_dev_test->ops;
}
#else
/* TODO How to fix empty translation unit in std99*/
int empty;
#endif

