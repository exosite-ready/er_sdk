/**
 * @file picocoap_client.c
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
 * @brief A protocol_client_class implementation for Picocoap
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/sf_malloc.h>
#include <lib/string_class.h>
#include <lib/debug.h>
#include <lib/pmatch.h>
#include <protocol_client.h>
#include <exosite_api_internal.h>
#include <client_config.h>
#include <lib/error.h>
#include <coap.h>

struct picocoap_context {
    struct net_socket *socket;
    struct string_class *request;
};

static void picocoap_delete_all(struct protocol_client_class *self)
{
    (void)self;
}

static int picocoap_send_request(struct protocol_client_class *self, void *pdu)
{
    size_t nsent;
    struct picocoap_context *ctx = self->ctx;
    struct string_class *request = pdu;
    int32_t error;

    ctx->request = pdu;
    error = self->net_if->send(ctx->socket, string_get_buffer(request),
            string_get_size(request), &nsent);

    if ((error == ERR_SUCCESS) && (nsent != string_get_size(request)))
        return ERR_COULD_NOT_SEND;

    return error;
}

#define RECV_BUFFER_LEN (128)
static int picocoap_recv_response(struct protocol_client_class *self,
        int *response_code, int *token, char *data, size_t max_size, size_t *payload_size)
{
    size_t recv_len;
    coap_error error;
    int32_t net_error;
    coap_pdu msg_recv = {NULL, 0, 64, NULL};
    uint8_t code_class, code_detail;
    char recv[RECV_BUFFER_LEN];
    struct picocoap_context *ctx = self->ctx;

    net_error = self->net_if->recv(ctx->socket, recv, RECV_BUFFER_LEN, &recv_len);
    if (net_error != ERR_SUCCESS)
        return net_error;

    msg_recv.buf = (unsigned char *) recv;
    msg_recv.len = recv_len;

    error = coap_validate_pkt(&msg_recv);
    if (error == CE_NONE) {
        struct coap_payload payload;

        code_class = coap_get_code_class(&msg_recv);
        code_detail = coap_get_code_detail(&msg_recv);
        *response_code = 100 * code_class + code_detail;
        /*printf("Pico COAP Code %d.%d\n", code_class, code_detail);*/
        payload = coap_get_payload(&msg_recv);
        if (max_size < payload.len)
            return ERR_OVERFLOW;

        if (data) {
            memcpy(data, (char *)payload.val, payload.len);
            data[payload.len] = '\0';
            *payload_size = payload.len;
        }

        *token = coap_get_token(&msg_recv);
        return ERR_SUCCESS;
    }

    return ERR_FAILURE;
}

static int picocoap_context_new(struct picocoap_context **ctx)
{
    struct picocoap_context *c;

    c = sf_malloc(sizeof(*c));
    if (!c)
        return ERR_NO_MEMORY;

    *ctx = c;
    return ERR_SUCCESS;
}

static void picocoap_set_socket(struct protocol_client_class *self,
        struct net_socket *socket)
{
    struct picocoap_context *ctx;

    ctx = (struct picocoap_context *) self->ctx;
    ctx->socket = socket;
}

static void picocoap_clear_socket(struct protocol_client_class *self)
{
    struct picocoap_context *ctx;

    ctx = (struct picocoap_context *) self->ctx;
    ctx->socket = NULL;
}

static BOOL picocoap_get_response_status(struct protocol_client_class *self, int32_t response_code)
{
    (void)self;
    if (response_code >= 200 && response_code < 300)
        return TRUE;

    return FALSE;
}

static struct protocol_client_ops rest_operations = {
        picocoap_set_socket,
        picocoap_clear_socket,
        picocoap_send_request,
        picocoap_recv_response,
        picocoap_delete_all,
        picocoap_get_response_status
};

void coap_client_new(struct protocol_client_class **obj,
        struct net_dev_operations *net_if)
{
    struct protocol_client_class *o;
    int status;
    struct picocoap_context *ctx;

    o = sf_malloc(sizeof(*o));
    if (!o)
        return;

    status = picocoap_context_new(&ctx);
    if (status != ERR_SUCCESS) {
        free(o);
        return;
    }

    o->ctx = ctx;
    o->net_if = net_if;
    o->ops = &rest_operations;
    *obj = o;
}

