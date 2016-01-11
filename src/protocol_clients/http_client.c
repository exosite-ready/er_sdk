/**
 * @file http_client.c
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
 * This file implements @ref protocol_client for HTTP
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/sf_malloc.h>
#include <lib/string_class.h>
#include <lib/debug.h>
#include <lib/pmatch.h>
#include <lib/error.h>
#include <porting/net_port.h>
#include <sdk_config.h>
#include <client_config.h>
#include <protocol_client.h>
#include "http_parser.h"

#define RECV_BUFFER_SIZE 256

enum http_type {
    REST_POST, REST_GET
};

struct http_context {
    struct net_socket *socket;
    struct string_class *request;
};

static int32_t http_send(struct protocol_client_class *self, void *pdu)
{
    size_t sent;
    int32_t error;
    struct string_class *request = pdu;

    error = self->net_if->send(self->socket, string_get_buffer(request),
            string_get_size(request), &sent);

    if ((error == ERR_SUCCESS) && (sent != string_get_size(request)))
        return ERR_COULD_NOT_SEND;

    return error;
}

static int32_t http_recv(struct protocol_client_class *self,
                                int32_t *response_code,
                                int32_t *token,
                                char *payload,
                                size_t max_size,
                                size_t *payload_size)
{
    size_t recvd;
    char buffer[RECV_BUFFER_SIZE];
    bool found_header = FALSE;
    int32_t error;
    size_t content_length = 0, payload_index = 0;

    error = self->net_if->recv(self->socket, buffer, RECV_BUFFER_SIZE, &recvd);
    if (error == ERR_SUCCESS) {
        if (!found_header) {
            size_t header_payload_index;

            found_header = http_parse_header(buffer, recvd,
                                             response_code,
                                             &content_length,
                                             &header_payload_index);

            if (found_header && header_payload_index) {
                size_t to_copy = recvd - header_payload_index;

                if (max_size < to_copy)
                    return ERR_OVERFLOW;
                if (payload)
                    memcpy(payload, buffer + header_payload_index, to_copy);

                *payload_size = to_copy;
                payload_index += to_copy;
            }
        }

        if (found_header) {
            if (content_length == 0 && payload && max_size > 0)
                payload[0] = '\0';

            error = ERR_SUCCESS;
        } else {
            error = ERR_FAILURE;
            debug_log(DEBUG_NET, ("Received %d bytes\n", recvd));
            error_log(DEBUG_NET, ("HTTP Header not found\n"), ERR_FAILURE);
            buffer[RECV_BUFFER_SIZE - 1] = '\0';
            debug_log(DEBUG_NET, ("HTTP packet: %s\n", buffer));
        }
    }

    *token = 1; /*HTTP MAGIC ID*/
    return error;
}

static void http_set_socket(struct protocol_client_class *self,
        struct net_socket *socket)
{
    self->socket = socket;
}

static void http_clear_socket(struct protocol_client_class *self)
{
    self->socket = NULL;
}

static void http_close(struct protocol_client_class *self)
{
    (void)self;
}

static BOOL http_get_response_status(struct protocol_client_class *self, int32_t response_code)
{
    (void)self;

    if (response_code >= 200 && response_code < 300)
        return TRUE;

    return FALSE;
}

static struct protocol_client_ops http_client_operations = {
        http_set_socket,
        http_clear_socket,
        http_send,
        http_recv,
        http_close,
        http_get_response_status
};

void http_client_new(struct protocol_client_class **obj, struct net_dev_operations *net_if)
{
    struct protocol_client_class *o;

    o = sf_malloc(sizeof(*o));
    if (!o)
        return;

    o->net_if = net_if;
    o->ops = &http_client_operations;
    *obj = o;
}
