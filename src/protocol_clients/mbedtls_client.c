/**
 * @file mbedtls_client.c
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
 * This module implements the @ref net_port_interface for MbedTLS
 **/

/*
 * MBEDTLS requires a NULL terminated string as the certificate!
 */
#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>

#include <porting/net_port.h>

#include "ca-cert.h"
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/use_ssl_heap.h>
#include <lib/sf_malloc.h>
#include <lib/utils.h>
#include <system_utils.h>
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/platform.h"
#include <ssl_client.h>


#define mbedtls_printf     printf
static struct net_dev_operations ssl_client_ops;
static struct net_dev_operations *net_if_ops;

static enum transport_protocol_type net_if_type;
#define IDENTITY_MAX_SIZE 128
#define PASSPHRASE_MAX_SIZE 128
static char net_if_identity[IDENTITY_MAX_SIZE];
static char net_if_passphrase[PASSPHRASE_MAX_SIZE];

/**
 * This is the private client object of ssl_client
 *
 * This implements a decorator like pattern, containing another
 * net_socket, that it will use
 * @TODO this structure could be turned into a more abstract
 * net_if_decorator object, for simplicity it is left like that
 **/
struct mbedtls_client_dev {
    struct net_socket self;
    struct net_socket *net_if;
    BOOL connected;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
};


/*Adapter functions for the MBED tls recv/send API*/
static int mbedtls_recv(void *ctx, unsigned char *buf, size_t size)
{
    struct net_socket *net_if = ctx;
    int32_t error;
    size_t nbytes_received;

    error = net_if_ops->recv(net_if, (char *)buf, (size_t)size, &nbytes_received);
    if (error == ERR_SUCCESS) {
        ASSERT(nbytes_received <= INT_MAX);
        return (int)nbytes_received;
    } else {
        if (error == ERR_WOULD_BLOCK)
            return MBEDTLS_ERR_SSL_WANT_READ;
        else
            return MBEDTLS_ERR_NET_RECV_FAILED;
    }
}

static int mbedtls_send(void *ctx, const unsigned char *buf, size_t size)
{
    int32_t error;
    size_t nbytes_sent;
    struct net_socket *net_if = ctx;

    error = net_if_ops->send(net_if, (const char *)buf, (size_t)size, &nbytes_sent);
    if (error == ERR_SUCCESS) {
        ASSERT(nbytes_sent <= INT_MAX);
        return (int)nbytes_sent;
    } else {
        if (error == ERR_WOULD_BLOCK)
            return MBEDTLS_ERR_SSL_WANT_READ;
        else
            return MBEDTLS_ERR_NET_RECV_FAILED;
    }
}

static void my_debug(void *ctx, int level,
                      const char *file, int line,
                      const char *str)
{
    ((void)level);
    (void)ctx;

    printf("%s:%04d: %s", file, line, str);
}

static int mbedtls_client_entropy_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    unsigned long timer = system_get_time();
    *olen = 0;

    if (len < sizeof(unsigned long))
        return 0;

    memcpy(output, &timer, sizeof(unsigned long));
    *olen = sizeof(unsigned long);

    return 0;
}

static int32_t mbedtls_client_init_context(struct mbedtls_client_dev *dev)
{
    const char *pers = "ssl_client1";
    int error;

    /*Set the debug threshold > 0 to get debug messages */
    mbedtls_debug_set_threshold(0);
    mbedtls_ssl_init(&dev->ssl);
    mbedtls_ssl_config_init(&dev->conf);
    mbedtls_x509_crt_init(&dev->cacert);
    mbedtls_ctr_drbg_init(&dev->ctr_drbg);

    mbedtls_entropy_init(&dev->entropy);
    mbedtls_entropy_add_source(&dev->entropy, mbedtls_client_entropy_poll, NULL,
                                4,
                                MBEDTLS_ENTROPY_SOURCE_STRONG);

    error = mbedtls_ctr_drbg_seed(&dev->ctr_drbg, mbedtls_entropy_func, &dev->entropy,
                                   (const unsigned char *)pers,
                                   strlen(pers));
    if (error != 0) {
        debug_log(DEBUG_SSL, (" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", error));
        goto exit;
    }

    error = mbedtls_x509_crt_parse(&dev->cacert, (const unsigned char *) ca_cert_pem, sizeof(ca_cert_pem));
    if (error < 0) {
        debug_log(DEBUG_SSL, (" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -error));
        goto exit;
    }

    error = mbedtls_ssl_config_defaults(&dev->conf,
                        MBEDTLS_SSL_IS_CLIENT,
                        MBEDTLS_SSL_TRANSPORT_STREAM,
                        MBEDTLS_SSL_PRESET_DEFAULT);
    if (error != 0) {
        debug_log(DEBUG_SSL, (" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", error));
        goto exit;
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example
     **/
    mbedtls_ssl_conf_authmode(&dev->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&dev->conf, &dev->cacert, NULL);
    mbedtls_ssl_conf_rng(&dev->conf, mbedtls_ctr_drbg_random, &dev->ctr_drbg);
    mbedtls_ssl_conf_dbg(&dev->conf, my_debug, stdout);
    mbedtls_ssl_conf_min_version(&dev->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

    error = mbedtls_ssl_setup(&dev->ssl, &dev->conf);
    if (error != 0) {
        debug_log(DEBUG_SSL, (" failed\n  ! mbedtls_ssl_setup returned %d\n\n", error));
        goto exit;
    }

    error = mbedtls_ssl_set_hostname(&dev->ssl, "mbed TLS Server 1");
    if (error != 0) {
        debug_log(DEBUG_SSL, (" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", error));
        goto exit;
    }

exit:
    return ERR_SUCCESS;
}

static int32_t mbedtls_client_socket(struct net_dev_operations *self, struct net_socket **nsocket)
{
    int32_t error;
    struct net_socket *socket;
    struct mbedtls_client_dev *dev;
    (void)self;
    (void)nsocket;

    dev = sf_malloc(sizeof(*dev));
    if (dev == NULL) {
        return ERR_NO_MEMORY;
    }

    mbedtls_client_init_context(dev);
    error = net_if_ops->socket(net_if_ops, &socket);
    if (error != ERR_SUCCESS) {

        return ERR_NO_MEMORY;
    }

    dev->net_if = socket;
    dev->self.ops = &ssl_client_ops;
    dev->connected = FALSE;

    mbedtls_ssl_set_bio(&dev->ssl, socket, mbedtls_send, mbedtls_recv, NULL);

    *nsocket = &dev->self;

    return ERR_SUCCESS;
}

static int32_t mbedtls_client_connect(struct net_socket *net_if, const char *host, unsigned short port)
{
    struct mbedtls_client_dev *dev;
    int32_t error;

    dev = container_of(net_if, struct mbedtls_client_dev, self);

    if (dev->connected)
        return ERR_SUCCESS;
    error = net_if_ops->connect(dev->net_if, host, port);
    if (error)
        return error;

    error = mbedtls_ssl_handshake(&dev->ssl);
    if (error != 0) {
        if (error == MBEDTLS_ERR_SSL_WANT_READ || error == MBEDTLS_ERR_SSL_WANT_WRITE)
            return ERR_WOULD_BLOCK;
        else
            return ERR_FAILURE;

    }
    debug_log(DEBUG_SSL, ("SSL version %s\n", mbedtls_ssl_get_version(&dev->ssl)));
    debug_log(DEBUG_SSL, ("SSL cipher suite is %s\n", mbedtls_ssl_get_ciphersuite(&dev->ssl)));
    dev->connected = TRUE;
    return ERR_SUCCESS;
}

static int32_t mbedtls_client_close(struct net_socket *net_if)
{
    struct mbedtls_client_dev *dev;

    dev = container_of(net_if, struct mbedtls_client_dev, self);

    mbedtls_ssl_close_notify(&dev->ssl);
    mbedtls_x509_crt_free(&dev->cacert);
    mbedtls_ssl_free(&dev->ssl);
    mbedtls_ssl_config_free(&dev->conf);
    mbedtls_ctr_drbg_free(&dev->ctr_drbg);
    mbedtls_entropy_free(&dev->entropy);
    net_if_ops->close(dev->net_if);
    sf_free(dev);
    dev->connected = FALSE;
    return 0;
}

static int32_t mbedtls_client_recv(struct net_socket *net_if, char *buf, size_t size, size_t *nbytes_received)
{
    struct mbedtls_client_dev *dev;
    int error;

    dev = container_of(net_if, struct mbedtls_client_dev, self);

    error = mbedtls_ssl_read(&dev->ssl, (unsigned char *)buf, size);
    if (error <= 0) {
        if (error == MBEDTLS_ERR_SSL_WANT_READ || error == MBEDTLS_ERR_SSL_WANT_WRITE)
            return ERR_WOULD_BLOCK;
        else
            return ERR_FAILURE;
    }

    *nbytes_received =  (size_t)error;
    return ERR_SUCCESS;
}

static int32_t mbedtls_client_send(struct net_socket *net_if, const char *buf, size_t size,  size_t *nbytes_sent)
{
    struct mbedtls_client_dev *dev;
    int32_t error;

    dev = container_of(net_if, struct mbedtls_client_dev, self);

    error = mbedtls_ssl_write(&dev->ssl, (unsigned const char *)buf, size);
    if (error <= 0) {
        if (error == MBEDTLS_ERR_SSL_WANT_READ || error == MBEDTLS_ERR_SSL_WANT_WRITE)
            return ERR_WOULD_BLOCK;
        else
            return ERR_FAILURE;
    }

    *nbytes_sent =  (size_t)error;
    return ERR_SUCCESS;
}

static int32_t mbedtls_client_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    (void)self;
    return net_if_ops->lookup(self, hostname, ipaddr_str, len);
}

static void *mbedtls_client_calloc(size_t n, size_t size)
{
    void *ptr;

    ptr = sf_malloc(n*size);
    if (ptr)
        memset(ptr, 0, n * size);

    return ptr;
}

static void mbedtls_client_free(void *ptr)
{
    if (ptr == NULL)
        return;

    sf_free(ptr);
}

unsigned long mbedtls_timing_hardclock(void);
unsigned long mbedtls_timing_hardclock(void)
{
    return system_get_time();
}

int32_t ssl_client_init(void)
{
    mbedtls_platform_set_calloc_free(mbedtls_client_calloc, mbedtls_client_free);
    return ERR_SUCCESS;
}

struct net_dev_operations *ssl_client_new(struct net_dev_operations *net_dev, enum transport_protocol_type type)
{
    net_if_ops = net_dev;
    net_if_type = type;

    return &ssl_client_ops;
}

static int32_t mbedtls_client_config(struct mac_addr *mac)
{
    return net_if_ops->config(mac);
}

static void mbedtls_client_deinit(void)
{
}

static int32_t mbedtls_client_set_identity(const char *identity, const char *passphrase)
{
    strncpy(net_if_identity, identity, IDENTITY_MAX_SIZE);
    strncpy(net_if_passphrase, passphrase, PASSPHRASE_MAX_SIZE);

    return ERR_SUCCESS;
}

static struct net_dev_operations ssl_client_ops = {
        mbedtls_client_socket,
        NULL,
        mbedtls_client_connect,
        NULL,
        mbedtls_client_close,
        mbedtls_client_recv,
        mbedtls_client_send,
        mbedtls_client_lookup,
        mbedtls_client_config,
        mbedtls_client_set_identity,
        mbedtls_client_deinit
};
