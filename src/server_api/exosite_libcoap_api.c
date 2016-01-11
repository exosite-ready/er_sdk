/**
 * @file exosite_libcoap_api.c
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
 * This file implements the Exosite Server specific Coap API for Libcoap
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/sf_malloc.h>
#include <lib/string_class.h>
#include <lib/debug.h>
#include <lib/error.h>
#include <lib/mem_check.h>
#include <exosite_api_internal.h>
#include <server_api.h>
#include <third_party/libcoap/coap.h>
#include <client_config.h>

static int order_opts(void *a, void *b)
{
    if (!a || !b)
        return a < b ? -1 : 1;

    if (COAP_OPTION_KEY(*(coap_option *) a)
            < COAP_OPTION_KEY(*(coap_option *) b))
        return -1;

    return COAP_OPTION_KEY(*(coap_option *) a)
            == COAP_OPTION_KEY(*(coap_option *) b);
}

static coap_list_t *
new_option_node(unsigned short key, unsigned int length, const unsigned char *data)
{
    coap_option *option;
    coap_list_t *node;

    option = coap_malloc(sizeof(coap_option) + length);
    if (!option)
        goto error;

    COAP_OPTION_KEY(*option) = key;
    COAP_OPTION_LENGTH(*option) = length;
    memcpy(COAP_OPTION_DATA(*option), data, length);

    /* we can pass NULL here as delete function since option is released automatically  */
    node = coap_new_listnode(option, NULL);

    if (node)
        return node;

error:
    perror("new_option_node: malloc");
    coap_free(option);
    return NULL;
}

static unsigned short message_id = 1;
static void coap_fill_request(coap_pdu_t *pdu, unsigned char m,
        coap_list_t *options, const char *payload, size_t payload_length, int token)
{
    coap_list_t *opt;

    pdu->hdr->type = COAP_MESSAGE_NON;
    pdu->hdr->id = COAP_HTONS(++(message_id));
    pdu->hdr->code = m;

    pdu->hdr->token_length = sizeof(token);
    if (!coap_add_token(pdu, sizeof(token), (unsigned char *)&token))
        debug("cannot add token to request\n");

    /*TODO: this is a temporary message until we debug out the COAP/DTLS server's token handling*/
    debug_log(DEBUG_NET, ("Libcoap set token to %d\n", token));
    /*ctx->sent_token = token;*/
    token++;

   /*coap_show_pdu(pdu);*/

    for (opt = options; opt; opt = opt->next) {
        coap_add_option(pdu, COAP_OPTION_KEY(*(coap_option *) opt->data),
                COAP_OPTION_LENGTH(*(coap_option *) opt->data),
                COAP_OPTION_DATA(*(coap_option *) opt->data));
    }

    if (payload_length) {
        /* if ((flags & FLAGS_BLOCK) == 0) */
        coap_add_data(pdu, payload_length, (const unsigned char *) payload);
        /*else coap_add_block(pdu, payload_length, payload, block.num, block.szx);*/
    }
}

static int coap_create_request(void *request, const struct exo_request_args *args)
{
    coap_list_t *optlist = NULL;
    coap_pdu_t *pdu = request;
    uint8_t method_code;
    const char *uri_path_rw = "1a";
    const char *uri_path_activate1 = "provision";
    const char *uri_path_activate2 = "activate";
    const char *uri_path_timestamp = "ts";
    size_t len = 0;

    if (args->value)
        len = strlen(args->value);

    switch (args->method) {
    case EXO_DP_READ:
    case EXO_DP_WRITE:
        method_code = ((args->method == EXO_DP_READ) ? 1 : 2);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(uri_path_rw),
                        (const unsigned char *) uri_path_rw), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(args->alias),
                        (const unsigned char *) args->alias), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_QUERY, strlen(args->cik),
                        (const unsigned char *) args->cik), order_opts);
        break;
    case EXO_PP_ACTIVATE:
        method_code = 2;
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH,
                        strlen(uri_path_activate1),
                        (const unsigned char *) uri_path_activate1), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH,
                        strlen(uri_path_activate2),
                        (const unsigned char *) uri_path_activate2), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(args->vendor),
                        (const unsigned char *) args->vendor), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(args->model),
                        (const unsigned char *) args->model), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(args->sn),
                        (const unsigned char *) args->sn), order_opts);
        break;
    case EXO_DP_SUBSCRIBE:
        method_code = 1;
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(uri_path_rw),
                        (const unsigned char *) uri_path_rw), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(args->alias),
                        (const unsigned char *) args->alias), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_QUERY, strlen(args->cik),
                        (const unsigned char *) args->cik), order_opts);
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_SUBSCRIPTION, 0, NULL), order_opts);
        break;
    case EXO_UP_TIMESTAMP:
        method_code = 1;
        coap_insert(&optlist,
                new_option_node(COAP_OPTION_URI_PATH, strlen(uri_path_timestamp),
                        (const unsigned char *) uri_path_timestamp), order_opts);
        break;
    default:
        return ERR_FAILURE;
    }

    coap_fill_request(pdu, method_code, optlist, args->value, len, args->message_id);
    /*ctx->method = method_code;*/
    coap_delete_list(optlist);
    /*ctx->optlist = NULL;*/
    return ERR_SUCCESS;
}

static int32_t coap_request_new(void **pdu, void *request_args)
{
    coap_pdu_t *coap_pdu;
    int32_t error;

    coap_pdu = coap_new_pdu();
    if (!coap_pdu)
        return ERR_FAILURE;

    error = coap_create_request(coap_pdu, request_args);
    if (error != ERR_SUCCESS)
        return error;

    *pdu = coap_pdu;
    return ERR_SUCCESS;
}

static int32_t coap_request_delete(void *pdu)
{
    coap_pdu_t *coap_pdu = pdu;

    coap_delete_pdu(coap_pdu);
    return ERR_SUCCESS;
}

static int32_t response_code_to_system_error(int32_t response_code, enum exosite_method method)
{
    if (response_code < 0)
        return response_code;

    switch (response_code) {
    case 204:
        if (method == EXO_DP_READ)
            return ERR_NO_CONTENT;
    case 200:
    case 205:
        return ERR_SUCCESS;
    case 304:
        return ERR_UNCHANGED;
    case 401:
        return ERR_CIK_NOT_VALID;
    case 404:
        return ERR_ACTIVATE_NOT_FOUND;
    case 409:
        return ERR_ACTIVATE_CONFLICT;
    default:
        return ERR_FAILURE;
    }
}

static int32_t coap_parse_payload(struct exo_request_args *args,
                                  char *payload,
                                  size_t size,
                                  size_t max_size,
                                  int32_t response_code)
{
    (void)payload;
    (void)size;
    (void)max_size;

    return response_code_to_system_error(response_code, args->method);
}

static struct server_api_request_ops server_api = {
        coap_request_new,
        coap_request_delete,
        coap_parse_payload
};

void exosite_coap_new(struct server_api_request_ops **obj)
{
    *obj = &server_api;
}

