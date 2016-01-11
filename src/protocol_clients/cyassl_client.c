/**
 * @file cyassl_client.c
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
 * This module implements the @ref net_port_interface for Cyassl
 **/

/*
 * Cyassl requires a data buffer for the certificate, it does not accept
 * terminating NULL at the end (when loading the certificate from a buffer
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stddef.h>
#include <time.h>
#include <limits.h>
#include <cyassl/ctaocrypt/types.h>
#include <cyassl/ctaocrypt/error-crypt.h>
#include <cyassl/ctaocrypt/settings.h>
#include <cyassl/ctaocrypt/logging.h>
#include <cyassl/ssl.h>
#include <porting/net_port.h>

#include "ca-cert.h"
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/use_ssl_heap.h>
#include <lib/sf_malloc.h>
#include <lib/utils.h>
#ifdef XMALLOC_USER
#include "cyassl_heap.h"
#endif
#include <ssl_client.h>


static struct net_dev_operations ssl_client_ops;

static struct net_dev_operations *net_if_ops;
static enum transport_protocol_type net_if_type;
#define IDENTITY_MAX_SIZE 128
#define PASSPHRASE_MAX_SIZE 128
static char net_if_identity[IDENTITY_MAX_SIZE];
static char net_if_passphrase[PASSPHRASE_MAX_SIZE];

extern time_t XTIME(time_t *timer);
/**
 * This is the private client object of ssl_client
 *
 * This implements a decorator like pattern, containing another
 * net_socket, that it will use
 * @TODO this structure could be turned into a more abstract
 * net_if_decorator object, for simplicity it is left like that
 **/
struct client_dev {
    struct net_socket self;
    struct net_socket *net_if;
    CYASSL *ssl;
    CYASSL_CTX *ctx;
};

#define caCert     "./certs/ca-cert.pem"

static int32_t map_cyassl_error_code(int32_t error)
{
    return map_error_code(ERR_MODULE_CYASSL, abs(error));
}

static void err_sys(const char *msg)
{
#ifndef EASDK_DEBUG_LEVEL_DEBUG
    (void)msg;
#endif
    debug_log(DEBUG_NET, ("Cyassl error: %s\n", msg));
}

static void showPeer(CYASSL *ssl)
{
    debug_log(DEBUG_NET, ("SSL version is %s\n", CyaSSL_get_version(ssl)));

    debug_log(DEBUG_NET,
            ("SSL cipher suite is %s\n", CyaSSL_CIPHER_get_name(CyaSSL_get_current_cipher(ssl))));

#if defined(SESSION_CERTS) && defined(SHOW_CERTS)
    {
        CYASSL_X509_CHAIN *chain = CyaSSL_get_peer_chain(ssl);
        int32_t count = CyaSSL_get_chain_count(chain);
        int32_t i;

        for (i = 0; i < count; i++) {
            int32_t length;
            unsigned char buffer[3072];
            CYASSL_X509 *chainX509;

            CyaSSL_get_chain_cert_pem(chain, i, buffer, sizeof(buffer), &length);
            buffer[length] = 0;
            printf("cert %d has length %d data =\n%s\n", i, length, buffer);

            chainX509 = CyaSSL_get_chain_X509(chain, i);
            if (chainX509)
            ShowX509(chainX509, "session cert info:");
            else
            printf("get_chain_X509 failed\n");
            CyaSSL_FreeX509(chainX509);
        }
    }
#endif
    (void) ssl;
}

static INLINE unsigned int exosite_psk_client_cb(CYASSL *ssl, const char *hint,
        char *identity, unsigned int id_max_len, unsigned char *key,
        unsigned int key_max_len)
{
    (void) ssl;
    (void) hint;

    /* identity is OpenSSL testing default for openssl s_client, keep same */
    strncpy(identity, net_if_identity, id_max_len);

    strncpy((char *)key, net_if_passphrase, key_max_len);

    return strlen(net_if_passphrase); /* length of key in octets or 0 for error */
}

/*Adapter functions for the cyassl recv/send API*/
static int cyassl_recv(CYASSL *ssl, char *buf, int sz, void *ctx)
{
    struct net_socket *net_if = ctx;
    int32_t error;
    size_t nbytes_received;

    (void)ssl;
    error = net_if_ops->recv(net_if, buf, (size_t)sz, &nbytes_received);
    if (error == ERR_SUCCESS) {
        ASSERT(nbytes_received <= INT_MAX);
        return (int)nbytes_received;
    } else {
        return error;
    }
}

static int cyassl_send(CYASSL *ssl, char *buf, int sz, void *ctx)
{
    int32_t error;
    size_t nbytes_sent;
    struct net_socket *net_if = ctx;

    (void)ssl;
    error = net_if_ops->send(net_if, buf, (size_t)sz, &nbytes_sent);
    if (error == ERR_SUCCESS) {
        ASSERT(nbytes_sent <= INT_MAX);
        return (int)nbytes_sent;
    } else {
        return error;
    }
}

/**
 * Factory function for ssl client
 * It returns an ssl client based on type, which can be ethernet and gsm/cdma modem at this point
 */
static int32_t client_socket(struct net_dev_operations *self, struct net_socket **nsocket)
{
    CYASSL_METHOD *method = 0;
    CYASSL_CTX *ctx = 0;
    CYASSL *ssl = 0;
    struct client_dev *dev;
    const char *cipherList = NULL;
    int32_t error;
#ifndef NO_FILESYSTEM
    const char *verifyCert = caCert;
#endif
    struct net_socket *socket;

    (void)self;

    if (net_if_type == TRANSPORT_PROTO_TCP)
        method = CyaTLSv1_2_client_method();
    else {
        cipherList = "PSK-AES128-CCM-8";
        method = CyaDTLSv1_2_client_method();
    }

    if (method == NULL) {
        err_sys("unable to get method");
        return ERR_FAILURE;
    }

    ctx = CyaSSL_CTX_new(method);
    if (ctx == NULL) {
        err_sys("unable to get ctx");
        return ERR_FAILURE;
    }

    if (cipherList) {
        if (CyaSSL_CTX_set_cipher_list(ctx, cipherList) != SSL_SUCCESS) {
            err_sys("client can't set cipher list 1");
            return ERR_FAILURE;
        }
    }

    if (net_if_type == TRANSPORT_PROTO_TCP) {
#ifndef NO_FILESYSTEM
        if (CyaSSL_CTX_load_verify_locations(ctx, verifyCert, 0) != SSL_SUCCESS)
            err_sys("can't load ca file, Please run from CyaSSL home dir");
#else
        /*
         * The certificate buffer contains a NULL terminated string, but cyassl can
         * not interpret it as a string, so provide size - 1 to cyassl, just the data
         */
        if (CyaSSL_CTX_load_verify_buffer(ctx, ca_cert_pem,
                        sizeof(ca_cert_pem) - 1, SSL_FILETYPE_PEM) != SSL_SUCCESS)
        err_sys("Can't load ca from buffer");
#endif
    } else {
        CyaSSL_CTX_set_psk_client_callback(ctx, exosite_psk_client_cb);
    }

    ssl = CyaSSL_new(ctx);
    if (ssl == NULL) {
        CyaSSL_CTX_free(ctx);
        err_sys("unable to get SSL object");
        return ERR_FAILURE;
    }

    dev = (struct client_dev *) XMALLOC(sizeof(*dev), 0, DYNAMIC_TYPE_SSL);
    if (dev == NULL) {
        CyaSSL_CTX_free(ctx);
        CyaSSL_free(ssl);
        return ERR_NO_MEMORY;
    }

    error = net_if_ops->socket(net_if_ops, &socket);
    if (error != ERR_SUCCESS) {
        CyaSSL_CTX_free(ctx);
        CyaSSL_free(ssl);
        XFREE(dev, 0, 0);
        return ERR_NO_MEMORY;
    }

    CyaSSL_SetIORecv(ctx, cyassl_recv);
    CyaSSL_SetIOSend(ctx, cyassl_send);

    CyaSSL_SetIOReadCtx(ssl, socket);
    CyaSSL_SetIOWriteCtx(ssl, socket);
    /**
     * quiet shutdown: do not send alert message that might not be sent before close
     * and unacked messaged will cause uip to discard close request
     **/
    /*
     * We don't want to turn on OPENSSL_EXTRA because it fails to compile on some embedded systems, but
     * we still want a quiet shutdown
     * This call is implemented in cyassl_adapt.c
     **/
    CyaSSL_set_quiet_shutdown(ssl, 1);

    dev->self.ops = &ssl_client_ops;
    dev->net_if = socket;
    dev->ssl = ssl;
    dev->ctx = ctx;

    *nsocket = &dev->self;
    return ERR_SUCCESS;
}

static int32_t client_close(struct net_socket *net_if)
{
    struct client_dev *dev;
    CYASSL *ssl;

    dev = container_of(net_if, struct client_dev, self);
    ssl = dev->ssl;
    CyaSSL_shutdown(ssl);
    CyaSSL_free(ssl);
    CyaSSL_CTX_free(dev->ctx);
    net_if_ops->close(dev->net_if);
    XFREE(dev, 0, 0);
    return 0;
}

static int32_t client_connect(struct net_socket *net_if, const char *host, unsigned short port)
{
    struct client_dev *dev;
    int32_t ret;
    CYASSL *ssl;
#ifdef EASDK_DEBUG_LEVEL_DEBUG
        char buffer[CYASSL_MAX_ERROR_SZ];
#endif

    dev = container_of(net_if, struct client_dev, self);
    ssl = dev->ssl;

    ret = net_if_ops->connect(dev->net_if, host, port);
    if (ret)
        return ret;

    if (CyaSSL_connect(ssl) != SSL_SUCCESS) {
        /* see note at top of README */
        int32_t err = CyaSSL_get_error(ssl, 0);

        if (err == SSL_ERROR_WANT_READ)
            return ERR_WOULD_BLOCK;
        if (err == SSL_ERROR_WANT_WRITE)
            return ERR_WOULD_BLOCK;

#ifdef EASDK_DEBUG_LEVEL_DEBUG
        debug_log(DEBUG_NET, ("err = %d, %s\n", err, CyaSSL_ERR_error_string((unsigned long)err, buffer)));
#endif

        err_sys("SSL_connect failed");
        /* if you're getting an error here  */
        return map_cyassl_error_code(err);
    } else {
        showPeer(ssl);
    }
    return ERR_SUCCESS;
}

static int32_t client_recv(struct net_socket *net_if, char *buf, size_t size, size_t *nbytes_received)
{
    int input;
    struct client_dev *dev;
    CYASSL *ssl;

    dev = container_of(net_if, struct client_dev, self);
    ssl = dev->ssl;
    ASSERT(size <= INT_MAX);
    input = CyaSSL_read(ssl, buf, (int)size);
    if (input < 0) {
        int32_t readErr = CyaSSL_get_error(ssl, 0);

        if (readErr != SSL_ERROR_WANT_READ) {
            err_sys("CyaSSL_read failed");
            debug_log(DEBUG_NET, ("Error: %d\n", readErr));
            return map_cyassl_error_code(readErr);
        } else {
            return ERR_WOULD_BLOCK;
        }
    }
    *nbytes_received = (size_t)input;
    return ERR_SUCCESS;
}

static int32_t client_send(struct net_socket *net_if, const char *buf, size_t size,  size_t *nbytes_sent)
{
    struct client_dev *dev;
    int sent_bytes;
    CYASSL *ssl;

    dev = container_of(net_if, struct client_dev, self);
    ssl = dev->ssl;
    ASSERT(size <= INT_MAX);
    sent_bytes = CyaSSL_write(ssl, buf, (int)size);
    if (sent_bytes != (int)size) {
        int writeErr = CyaSSL_get_error(ssl, 0);

        if (writeErr != SSL_ERROR_WANT_WRITE) {
            err_sys("CyaSSL_write failed");
            return map_cyassl_error_code(writeErr);
        } else {
            return ERR_WOULD_BLOCK;
        }
    }

    *nbytes_sent = (size_t)sent_bytes;
    return ERR_SUCCESS;
}

static int32_t client_lookup(struct net_dev_operations *self, char *hostname, char *ipaddr_str, size_t len)
{
    (void)self;
    return net_if_ops->lookup(self, hostname, ipaddr_str, len);
}

static void client_logging_cb(const int logLevel, const char * const logMessage)
{
    if (logLevel <= INFO_LOG) {
        /*Do not write messages with cyassl internal error code WANT_READ=323*/
        if (logLevel == ERROR_LOG && strstr(logMessage, "323"))
            return;

        debug_log(DEBUG_NET, ("%s\n", logMessage));
    }
}

static void *cy_malloc(size_t size)
{
    return sf_malloc(size);
}

static void cy_free(void *p)
{
    sf_free(p);
}

static void *cy_realloc(void *p, size_t size)
{
    return sf_realloc(p, size);
}

int32_t ssl_client_init(void)
{
    int32_t error;

#ifdef XMALLOC_USER
    cyassl_heap_init();
#else
    CyaSSL_SetAllocators(cy_malloc, cy_free, cy_realloc);
#endif

    error = CyaSSL_Init();
    if (error != SSL_SUCCESS) {
        error_log(DEBUG_NET, ("Cyassl init failed\n"), error);
        return ERR_FAILURE;
    }

    CyaSSL_SetLoggingCb(client_logging_cb);
#if defined(DEBUG_CYASSL) && !defined(CYASSL_MDK_SHELL) && !defined(STACK_TRAP)
    CyaSSL_Debugging_ON();
#endif
#ifndef NO_FILESYSTEM
    if (CurrentDir("client"))
        ChangeDirBack(2);
    else if (CurrentDir("Debug") || CurrentDir("Release"))
        ChangeDirBack(3);
#endif
    return ERR_SUCCESS;
}

struct net_dev_operations *ssl_client_new(struct net_dev_operations *net_dev, enum transport_protocol_type type)
{
    net_if_ops = net_dev;
    net_if_type = type;

    return &ssl_client_ops;
}

static int32_t client_config(struct mac_addr *mac)
{
    return net_if_ops->config(mac);
}

static void client_deinit(void)
{
    CyaSSL_Cleanup();
}

static int32_t client_set_identity(const char *identity, const char *passphrase)
{
    strncpy(net_if_identity, identity, IDENTITY_MAX_SIZE);
    strncpy(net_if_passphrase, passphrase, PASSPHRASE_MAX_SIZE);

    return ERR_SUCCESS;
}

static struct net_dev_operations ssl_client_ops = {
        client_socket,
        NULL,
        client_connect,
        NULL,
        client_close,
        client_recv,
        client_send,
        client_lookup,
        client_config,
        client_set_identity,
        client_deinit
};
