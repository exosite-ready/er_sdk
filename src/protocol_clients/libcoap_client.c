/**
 * @file libcoap_client.c
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
 * This file implements @ref protocol_client for Libcoap
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
#include <third_party/libcoap/coap.h>
#include <sdk_config.h>
#include <client_config.h>
#include <protocol_client.h>

#define WITHOUT_BLOCK

static int32_t coap_pdu_copy(coap_pdu_t **npdu, coap_pdu_t *pdu)
{
    int32_t error;
    coap_pdu_t *new_pdu;

    error = sf_mem_alloc((void **)&new_pdu, pdu->max_size);
    if (error != ERR_SUCCESS)
        return error;

    memcpy(new_pdu, pdu, pdu->max_size);
    new_pdu->hdr = (coap_hdr_t *)(void *)((unsigned char *)new_pdu + sizeof(coap_pdu_t));
    *npdu = new_pdu;

    return ERR_SUCCESS;
}

static int32_t coap_send_request(struct protocol_client_class *self, void *pdu)
{
    coap_tid_t tid = COAP_INVALID_TID;
    coap_pdu_t *request; /* = pdu; */
    BOOL reliable = TRUE;
    int32_t error;

    error = coap_pdu_copy(&request, pdu);
    if (error != ERR_SUCCESS)
        return error;

    if (reliable) {
        tid = coap_send_confirmed(self->ctx, NULL, request);
        if (tid == COAP_INVALID_TID) {
            debug_log(DEBUG_NET, ("Coap: error sending new request"));
            coap_delete_pdu(pdu);
        }
    } else {
        tid = coap_send(self->ctx, NULL, request);
        coap_delete_pdu(request);
    }

    /*if (request->hdr->type != COAP_MESSAGE_CON || tid == COAP_INVALID_TID)*/
    /*return FALSE;*/
    if (tid == COAP_INVALID_TID)
        return ERR_FAILURE;

    return ERR_SUCCESS;
}

unsigned int obs_seconds = 30; /* default observe time */
coap_tick_t obs_wait; /* 0 timeout for current subscription */
unsigned int wait_seconds = 90; /* default timeout in seconds */
coap_tick_t max_wait; /* global timeout (changed by set_timeout()) */

static int append_to_output(coap_context_t *ctx, const unsigned char *data,
        size_t len)
{
    char *tmp;

    if (ctx->payload == NULL) {
        ctx->payload = sf_malloc(len);
        if (ctx->payload == NULL)
            return -1;
        ctx->payload_len = len;
    } else {
        if (len > ctx->payload_len) {
            tmp = sf_realloc(ctx->payload, len);
            if (tmp) {
                ctx->payload = tmp;
                ctx->payload_len = len;
            } else {
                return -1;
            }
        }
    }
    sf_memcpy(ctx->payload, data, len);
    check_memory();

    return 0;
}

/*Returns TRUE if check is OK*/
static int check_token(coap_pdu_t *received)
{
    (void)received;
#ifdef EASDK_DEBUG_LEVEL_DEBUG
    unsigned char *t = received->hdr->token;
#endif
    debug_log(DEBUG_NET, ("Received token %d\n", *t));
    return TRUE;
}

static inline void set_timeout(coap_tick_t *timer, const unsigned int seconds)
{
    coap_ticks(timer);
    *timer += seconds * COAP_TICKS_PER_SECOND;
}

static inline coap_opt_t *
get_block(coap_pdu_t *pdu, coap_opt_iterator_t *opt_iter)
{
    coap_opt_filter_t f;

    assert(pdu);

    memset(f, 0, sizeof(coap_opt_filter_t));
    coap_option_setb(f, COAP_OPTION_BLOCK1);
    coap_option_setb(f, COAP_OPTION_BLOCK2);

    coap_option_iterator_init(pdu, opt_iter, f);
    return coap_option_next(opt_iter);
}

#ifndef WITHOUT_BLOCK
static coap_pdu_t *
coap_new_request(coap_context_t *ctx, unsigned char m, coap_list_t *options)
{
    coap_pdu_t *pdu;

    pdu = coap_new_pdu();
    if (!pdu)
        return NULL;

    coap_fill_request(ctx, pdu, m, options, NULL, 0);

    return pdu;
}
#endif

static void message_handler(struct coap_context_t *ctx,
        const coap_address_t *remote, coap_pdu_t *sent, coap_pdu_t *received,
        const coap_tid_t id)
{
    coap_pdu_t *pdu = NULL;
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
#ifndef WITHOUT_BLOCK
    unsigned char buf[4];
    coap_list_t *option;
    coap_tid_t tid;
#endif
    size_t len;
    unsigned char *databuf;

    (void)id;
    /* coap_show_pdu(received); */

#ifndef NDEBUG
    if (coap_get_log_level() >= LOG_DEBUG) {
        debug("** process incoming %d.%02d response:\n",
                (received->hdr->code >> 5), received->hdr->code & 0x1F);
        coap_show_pdu(received);
    }
#endif

    /* check if this is a response to our original request */
#if 1
    if (!check_token(received)) {
        /* drop if this was just some message, or send RST in case of notification */
        if (!sent
                && (received->hdr->type == COAP_MESSAGE_CON
                        || received->hdr->type == COAP_MESSAGE_NON))
            coap_send_rst(ctx, remote, received);
        return;
    }
#endif

    switch (received->hdr->type) {
    case COAP_MESSAGE_CON:
        /* acknowledge received response if confirmable (TODO: check Token) */
        coap_send_ack(ctx, remote, received);
        break;
    case COAP_MESSAGE_RST:
        info("got RST\n");
        return;
    default:
        break;
        ;
    }

    /* output the received data, if any */
    ctx->code = received->hdr->code;
    ctx->received_token = *((int *) (void *)received->hdr->token);
    if (received->hdr->code == COAP_RESPONSE_CODE(205)) {

        /* set obs timer if we have successfully subscribed a resource */
        if (sent
                && coap_check_option(received, COAP_OPTION_SUBSCRIPTION,
                        &opt_iter)) {
            debug("observation relationship established, set timeout to %d\n",
                    obs_seconds);
            debug_log(DEBUG_NET, ("observation relationship established\n"));
            set_timeout(&obs_wait, obs_seconds);
        }

        /*
         * Got some data, check if block option is set. Behavior is undefined if
         * both, Block1 and Block2 are present.
         **/
        block_opt = get_block(received, &opt_iter);
        if (!block_opt) {
            /* There is no block option set, just read the data and we are done. */
            if (coap_get_data(received, &len, &databuf))
                append_to_output(ctx, databuf, len);
        } else {
#ifndef WITHOUT_BLOCK
            unsigned short blktype = opt_iter.type;

            /* TODO: check if we are looking at the correct block number */
            if (coap_get_data(received, &len, &databuf))
                append_to_output(ctx, databuf, len);

            if (COAP_OPT_BLOCK_MORE(block_opt)) {
                /* more bit is set */

                debug("found the M bit, block size is %u, block nr. %u\n",
                        COAP_OPT_BLOCK_SZX(block_opt),
                        coap_opt_block_num(block_opt));

                /* create pdu with request for next block */
                pdu = coap_new_request(ctx, ctx->method, NULL); /* first, create bare PDU w/o any option  */
                if (pdu) {
                    /* add URI components from optlist */
                    for (option = ctx->optlist; option; option = option->next) {
                        switch (COAP_OPTION_KEY(*(coap_option *) option->data)) {
                        case COAP_OPTION_URI_HOST:
                        case COAP_OPTION_URI_PORT:
                        case COAP_OPTION_URI_PATH:
                        case COAP_OPTION_URI_QUERY:
                            coap_add_option(pdu,
                                    COAP_OPTION_KEY(
                                            *(coap_option *) option->data),
                                    COAP_OPTION_LENGTH(
                                            *(coap_option *) option->data),
                                    COAP_OPTION_DATA(
                                            *(coap_option *) option->data));
                            break;
                        default:
                            break;
                            ; /* skip other options */
                        }
                    }

                    /* finally add updated block option from response, clear M bit */
                    /* blocknr = (blocknr & 0xfffffff7) + 0x10; */
                    debug("query block %d\n",
                            (coap_opt_block_num(block_opt) + 1));
                    coap_add_option(pdu, blktype,
                            coap_encode_var_bytes(buf,
                                    ((coap_opt_block_num(block_opt) + 1) << 4)
                                            | COAP_OPT_BLOCK_SZX(block_opt)),
                            buf);

                    if (received->hdr->type == COAP_MESSAGE_CON)
                        tid = coap_send_confirmed(ctx, remote, pdu);
                    else
                        tid = coap_send(ctx, remote, pdu);

                    if (tid == COAP_INVALID_TID) {
                        debug("message_handler: error sending new request");
                        coap_delete_pdu(pdu);
                    } else {
                        set_timeout(&max_wait, wait_seconds);
                        if (received->hdr->type != COAP_MESSAGE_CON)
                            coap_delete_pdu(pdu);
                    }

                    return;
                }
            }
#endif
        }
    } else { /* no 2.05 */

        /* check if an error was signaled and output payload if so */
        if (COAP_RESPONSE_CLASS(received->hdr->code) >= 4) {
            fprintf(stderr, "%d.%02d", (received->hdr->code >> 5),
                    received->hdr->code & 0x1F);
            if (coap_get_data(received, &len, &databuf)) {
                fprintf(stderr, " ");
                while (len--)
                    fprintf(stderr, "%c", *databuf++);
            }
            fprintf(stderr, "\n");
        }

    }

    /* finally send new request, if needed */
    if (pdu && coap_send(ctx, remote, pdu) == COAP_INVALID_TID)
        debug("message_handler: error sending response");
    if (pdu)
        coap_delete_pdu(pdu);

    /* our job is done, we can exit at any time */
    ctx->ready = TRUE;
#if 1
    if (coap_check_option(received, COAP_OPTION_SUBSCRIPTION, &opt_iter) &&
           received->hdr->type == COAP_MESSAGE_ACK)
        ctx->ready = FALSE;
#endif

}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
static int32_t coap_recv_response(struct protocol_client_class *self, int32_t *response_code, int32_t *token,
        char *data, size_t max_len, size_t *payload_size)
{
    coap_tick_t now;
    coap_queue_t *nextpdu;
    struct coap_context_t *ctx = self->ctx;
    int ret;

    (void)payload_size;
    nextpdu = coap_peek_next(ctx);
    coap_ticks(&now);
    while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
        coap_retransmit(ctx, coap_pop_next(ctx));
        nextpdu = coap_peek_next(ctx);
    }

    ret = coap_read(ctx);
    if (ret == ERR_WOULD_BLOCK)
        return ret;

    coap_dispatch(ctx);

    if (ctx->ready) {
        if (data) {
            size_t to_copy = MIN(ctx->payload_len, max_len - 1);

            memcpy(data, ctx->payload, to_copy);
            data[to_copy] = '\0';
            if (ctx->payload)
                sf_free(ctx->payload);
            ctx->payload_len = 0;
            ctx->payload = NULL;
        }

        int detail = ctx->code & 0x1f;
        *response_code = COAP_RESPONSE_CLASS(ctx->code) * 100 + detail;
        *token = ctx->received_token;

        return ERR_SUCCESS;
    } else
        return ERR_WOULD_BLOCK;
}

static void coap_set_socket(struct protocol_client_class *self,
        struct net_socket *socket)
{
    coap_context_t *ctx;

    ctx = (coap_context_t *) self->ctx;
    ctx->socket = socket;
}

static void coap_clear_socket(struct protocol_client_class *self)
{
    coap_context_t *ctx;

    ctx = (coap_context_t *) self->ctx;
    ctx->socket = NULL;
}

static void coap_request_delete_all(struct protocol_client_class *self)
{
    struct coap_context_t *ctx = self->ctx;

    coap_free_context(ctx);
}

static BOOL coap_get_response_status(struct protocol_client_class *self, int32_t response_code)
{
    (void)self;
    if (response_code >= 200 && response_code < 300)
        return TRUE;

    return FALSE;
}

static struct protocol_client_ops coap_client_operations = {
        coap_set_socket,
        coap_clear_socket,
        coap_send_request,
        coap_recv_response,
        coap_request_delete_all,
        coap_get_response_status
};

void coap_client_new(struct protocol_client_class **obj,
        struct net_dev_operations *net_if)
{
    struct coap_context_t *ctx;
    struct protocol_client_class *o;

    o = sf_malloc(sizeof(*o));
    if (!o)
        return;

    /* coap_set_log_level(LOG_DEBUG); */
    o->net_if = net_if;
    o->ops = &coap_client_operations;
    ctx = coap_new_context(NULL);
    coap_register_response_handler(ctx, message_handler);
    ctx->net_if = net_if;
    o->ctx = ctx;

    *obj = o;
}

