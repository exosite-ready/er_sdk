/**
 * @file exosite_picocoap_api.c
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
 * This file implements the Exosite Server specific Coap API for Picocoap
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
#include <client_config.h>
#include <coap.h>


static int message_id = 1;
static int32_t coap_create_request(void *request, const struct exo_request_args *args)
{
#define MSG_BUF_LEN 128
    uint8_t msg_send_buf[MSG_BUF_LEN];
    coap_pdu msg_send = { msg_send_buf, 0, MSG_BUF_LEN, NULL };
    const char *uri_path_activate1 = "provision";
    const char *uri_path_activate2 = "activate";
    uint8_t obs_opt = 0;

    coap_init_pdu(&msg_send);
    coap_set_version(&msg_send, COAP_V1);
    coap_set_type(&msg_send, CT_NON);

    switch (args->method) {
    case EXO_DP_READ:
        coap_set_code(&msg_send, CC_GET);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) "1a", 2);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->alias, strlen(args->alias));
        coap_add_option(&msg_send, CON_URI_QUERY, (uint8_t *) args->cik, strlen(args->cik));
        break;
    case EXO_DP_WRITE:
        coap_set_code(&msg_send, CC_POST);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) "1a", 2);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->alias, strlen(args->alias));
        coap_add_option(&msg_send, CON_URI_QUERY, (uint8_t *) args->cik, strlen(args->cik));
        coap_set_payload(&msg_send, (uint8_t *) args->value, strlen(args->value));
        break;
    case EXO_PP_ACTIVATE:
        coap_set_code(&msg_send, CC_POST);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) uri_path_activate1, strlen(uri_path_activate1));
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) uri_path_activate2, strlen(uri_path_activate2));
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->vendor, strlen(args->vendor));
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->model, strlen(args->model));
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->sn, strlen(args->sn));
        break;
    case EXO_DP_SUBSCRIBE:
        coap_set_type(&msg_send, CT_CON);
        coap_set_code(&msg_send, CC_GET);
        coap_add_option(&msg_send, CON_OBSERVE, &obs_opt, 1);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) "1a", 2);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) args->alias, strlen(args->alias));
        coap_add_option(&msg_send, CON_URI_QUERY, (uint8_t *) args->cik, strlen(args->cik));
        break;
    case EXO_UP_TIMESTAMP:
        coap_set_code(&msg_send, CC_GET);
        coap_add_option(&msg_send, CON_URI_PATH, (uint8_t *) "ts", 2);
        break;
    default:
        return ERR_FAILURE;
    }

   coap_set_mid(&msg_send, message_id);
   coap_set_token(&msg_send, args->message_id, 4);

    string_catcn(request, (char *) msg_send.buf, msg_send.len);
    return ERR_SUCCESS;
}

static int coap_request_new(void **pdu, void *request_args)
{
    int error;
    struct string_class *request;

    error = string_new(&request, "");
    if (error != ERR_SUCCESS)
        return error;

    error = coap_create_request(request, request_args);
    if (error != ERR_SUCCESS)
        return error;

    *pdu = request;
    return ERR_SUCCESS;
}

static int coap_request_delete(void *pdu)
{
    struct string_class *str = pdu;

    string_delete(str);

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
