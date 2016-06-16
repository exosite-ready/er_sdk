/**
 * @file client_config.c
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
 * This file contains client configuration structures
 *
 * New protocols, server API implementations and server configurations can be added to the SDK
 * through this file
 **/

#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <porting/net_port.h>
#include <protocol_client_common.h>
#include <exosite_api_internal.h>
#include <net_dev_factory.h>
#include <lib/heap_types.h>
#include <client_config.h>

/* If FORCED_ACTIVATION is set, the device sends an
 * activate request. If the request successes the stored
 * CIK will be overwrite with the new one.
 * If FORCED_ACTIVATION is not set, the device tries to
 * load the CIK from the permanent storage first.
 **/


/**
 * Try to connect for this long
 **/
#define CONNECT_TIMEOUT          (35000)

/**
 * If there is an error during connect
 * retry connect this many times before closing and reopening the socket
 **/
#define CONNECT_RETRIES (5)

/* TODO TRANSMISSION_RETRIES has to be 0 because libCOAP client frees its request (pdu)
 * if it did not, then we need to find a way to clean up the pdu only
 * in case it was not successfully sent
 **/
/**
 * If there is an error during send/receive
 * Retry send/receive this many times before closing and reopening the socket
 **/
#define TRANSMISSION_RETRIES    (0)

/**
 * The protocol client sm accepts this many requests
 **/
#define PROTOCOL_CLIENT_SM_QUEUE_SIZE (1)
/* TODO
 * HTTP long poll timeout seem to be harcodede to ~30s
 * We should not time out before that
 **/

struct transmission_config_class transmission_config = {
    CONNECT_RETRIES,
    TRANSMISSION_RETRIES,
    CONNECT_TIMEOUT,
    RESPONSE_TIMEOUT,
    MAX_REQUEST_QUEUE_SIZE,
    PROTOCOL_CLIENT_SM_QUEUE_SIZE,
    NO_REQUEST_LIMIT,                   /* Parallel requests */
#if (CONFIG_SECURITY != cfg_no_security)
    SECURITY_TLS_DTLS,                  /* Security mode */
#else
    SECURITY_OFF,
#endif
    TRUE                                /* Forced activation */
};

/* period time in [ms] */
#define RTC_PERIOD                (1000)
#define STATE_MACHINE_PERIOD      (10)

struct periodic_config_class periodic_config = {
    RTC_PERIOD,
    STATE_MACHINE_PERIOD
};

/**
 * Server names and ports for the different Application protocols/security can be configured here
 * Make sure that you fill the server_data array as well
 **/
static struct server_data_class server_data_http  = {APP_PROTO_HTTP, SECURITY_OFF, "m2.exosite.com", 80};
static struct server_data_class server_data_https = {APP_PROTO_HTTP, SECURITY_TLS_DTLS, "m2.exosite.com", 443};
static struct server_data_class server_data_coap  = {APP_PROTO_COAP, SECURITY_OFF, "coap.exosite.com", 5683};
static struct server_data_class server_data_coaps = {APP_PROTO_COAP, SECURITY_TLS_DTLS, "coap-dev.exosite.com", 5684};

static struct server_data_class *server_data[] = {
        &server_data_http,
        &server_data_https,
        &server_data_coap,
        &server_data_coaps,
        NULL
};

/**
 * New Server APIs can be added here
 * Make sure you fill the server_apis array as well
 **/
static struct server_api_class exosite_http_api = {APP_PROTO_HTTP, "Exosite HTTP", exosite_http_api_request_new, NULL};
static struct server_api_class exosite_coap_api = {APP_PROTO_COAP, "Exosite COAP", exosite_coap_new, NULL};

static struct server_api_class *server_apis[] = {
        &exosite_http_api,
        &exosite_coap_api,
        NULL
};

/**
 * New Application protocols can be added here
 * Make sure you fill the protocol_client_configs array as well
 **/
static struct protocol_client_config_class http_config = {
        APP_PROTO_HTTP, "HTTP", http_client_new, TRANSPORT_PROTO_TCP, SM_SYNCH, 1, NULL
};
static struct protocol_client_config_class coap_config = {
        APP_PROTO_COAP, "COAP", coap_client_new, TRANSPORT_PROTO_UDP, SM_ASYNCH, NO_REQUEST_LIMIT, NULL
};

static struct protocol_client_config_class *protocol_client_configs[] = {
        &http_config,
        &coap_config,
        NULL
};


struct transmission_config_class *client_config_get_transmission_config(void)
{
    return &transmission_config;
}

struct periodic_config_class *client_config_get_periodic_config(void)
{
    return &periodic_config;
}

struct server_data_class **client_config_get_server_data(void)
{
    return &server_data[0];
}

int32_t client_config_get_protocol_client_config(struct protocol_client_config_class **client_config,
        enum application_protocol_type type, enum security_mode security)
{
    struct net_dev_operations *net_dev;
    int32_t i;
    int32_t error;

    for (i = 0; protocol_client_configs[i] != NULL; i++)
        if (protocol_client_configs[i]->appproto_type == type)
            break;

    if (protocol_client_configs[i] == NULL) {
        /* REQUESTED PROTOCOL was not registered*/
        ASSERT(0);
        error_log(DEBUG_PRC_SM, ("Protocol client not found\n"), ERR_NOT_FOUND);
        return ERR_NOT_FOUND;
    }

    debug_log(DEBUG_NET, ("Initalizing %s\n", protocol_client_configs[i]->name));
    error = net_dev_get(&net_dev, protocol_client_configs[i]->tproto_type, security);
    if (error != ERR_SUCCESS)
        return error;

    protocol_client_configs[i]->new(&protocol_client_configs[i]->exo_api, net_dev);

    *client_config = protocol_client_configs[i];
    return ERR_SUCCESS;
}


int32_t client_config_get_server_api(struct server_api_request_ops **new_request_ops,
                                     enum application_protocol_type type)
{
    int32_t i;
    struct server_api_request_ops *ops;

    for (i = 0; server_apis[i] != NULL; i++)
        if (server_apis[i]->appproto_type == type)
            break;

    if (i == sizeof(server_apis) / sizeof(struct server_api_class *) - 1) {
        /* Requested server API was not registered */
        ASSERT(0);
        error_log(DEBUG_PRC_SM, ("Server API request class not found\n"), ERR_NOT_FOUND);
        return ERR_NOT_FOUND;
    }

    server_apis[i]->new(&ops);

    *new_request_ops = ops;
    return ERR_SUCCESS;
}

/**
 * To configure more heaps first you have to add a heap type in heap_types.h then you should add
 * a configuration entry here
 *
 * After that you can start using the new heap
 **/
#if CONFIG_HAS_STRING_HEAP == 1
static struct heap_config string_heap_config = {CONFIG_STRING_HEAP_SIZE, "String", STRING_HEAP};
#endif
#if CONFIG_HAS_SSL_HEAP == 1
static struct heap_config ssl_heap_config = {CONFIG_SSL_HEAP_SIZE, "SSL", SSL_HEAP};
#endif

static struct heap_config *global_heap_config[] = {
#if CONFIG_HAS_STRING_HEAP == 1
        &string_heap_config,
#endif
#if CONFIG_HAS_SSL_HEAP == 1
        &ssl_heap_config,
#endif
        NULL
};

struct heap_config **client_config_get_heap_config(void)
{
    return &global_heap_config[0];
}
