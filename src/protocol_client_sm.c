/**
 * @file protocol_client_sm.c
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
 * This implements an application layer / protocol layer independent messaging with the server
 *
 * This module uses protocol clients implementing @ref protocol_client. It does not know about
 * any underlying details about
 *   - the protocol (HTTP, COAP, etc)
 *   - the Server API, that defines how to communicate with the server through a certain protocol
 *
 * Protocol clients can be added in client_config.c
 *
 * It gets an already assembled request and tries to send it using @ref protocol_client
 * It will call the provided callback
 *    - If a corresponding response is received
 *    - The specified response timeout has elapsed
 *
 * So it is guaranteed that the application gets called within Response timeout.
 */

#include <stdio.h>
#include <string.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/string_class.h>
#include <lib/e_list.h>
#include <lib/concurrent_queue.h>
#include <lib/mem_check.h>
#include <lib/pclb.h>

#include <porting/thread_port.h>
#include <porting/net_port.h>
#include <porting/system_port.h>

#include <system_utils.h>
#include <client_config.h>
#include <protocol_client.h>
#include <protocol_client_sm.h>

enum protocol_client_sm_state {
    SM_RESET = 0,
    SM_CONNECT,
    SM_TRANSMIT,
    SM_FAILURE,
};

#define MAX_RESPONSE_TIMEOUT_PER_SESSION 5

/**
 * The struct protocol_client_request contains the request that will be sent by
 * protocol_client->send_request
 * Once protocol_client->send_request is called, the request is owned by the protocol_client
 **/
struct protocol_client_request {
    enum request_type type; /* Type of the request to distinguish between plain request and subscribe */
    void *request;          /* The request; this will be created by the API using a registered server api */
    void *request_args;
    /* Callback when a response is received or when
     * request times out
     */
    pc_sm_callback clb;
    sys_time_t start_of_transmit;  /* The time when the request is popped from the queue*/
    int32_t message_id; /* Response and request are paired based on the ID of the request */
    struct list_head request_list;
};

struct protocol_client_sm_class {
    const char *name;
    struct net_dev_operations *net_if;
    char *server_name;
    char host_ip[18];
    uint16_t server_port;
    bool connected;
    bool always_disconnect;
    struct net_socket *socket;
    struct protocol_client_class *protocol_client;
    CQueue *transfer_queue;            /* Requests to send are put on this queue */
    struct list_head sent_request_list; /* Put successfully sent transfers on this list */
    int32_t sent_request_num; /* Number of the sent requests (items in sent_request_list) */
    int32_t paralell_request_limit;   /* Number of the requests can be sent parallel */
    struct protocol_client_request *active_request;
    struct protocol_client_request *completed_request;
    struct protocol_client_response response;
    enum protocol_client_sm_state state;
    int32_t retry_cnt;
    int32_t transmit_failures;
    enum state_machine_type type;
    struct transmission_config_class *transmission_config;
    BOOL abort_transfers_in_progress;
    BOOL abort_done;
    system_mutex_t sm_mutex;
    sys_time_t last_reset;
    int32_t timeouts;             /* In the current socket session */
};

/*A flag to serialize socket connects in all the state machine instances*/
static struct protocol_client_sm_class *connecting_sm;
static system_mutex_t connect_mtx; /*The mutex to protect this flag*/

static BOOL bool_get_atomic(system_mutex_t mtx, BOOL *b)
{
    BOOL temp;

    system_mutex_lock(mtx);
    temp = *b;
    system_mutex_unlock(mtx);

    return temp;
}

static void bool_set_atomic(system_mutex_t mtx, BOOL *b, BOOL value)
{
    system_mutex_lock(mtx);
    *b = value;
    system_mutex_unlock(mtx);
}

static int32_t connect_wait(struct protocol_client_sm_class *sm)
{
    int32_t state;

    system_mutex_lock(connect_mtx);
    if (connecting_sm == NULL || connecting_sm == sm) {
        /*debug_log(DEBUG_TRACE, DEBUG_PRC_SM, ("Connect wait %d\n", connect_in_progress));*/
        connecting_sm = sm;
        state = ERR_SUCCESS;
   } else
       state = ERR_BUSY;

    system_mutex_unlock(connect_mtx);
    return state;
}

static void connect_post(void)
{
    system_mutex_lock(connect_mtx);

    connecting_sm = NULL;

    system_mutex_unlock(connect_mtx);
}

void protocol_client_sm_init(void)
{
    system_mutex_create(&connect_mtx);
    connecting_sm = NULL;
}

static int32_t protocol_client_request_new(struct protocol_client_request **request)
{
    int32_t error;

    error = sf_mem_alloc((void **)request, sizeof(struct protocol_client_request));
    if (error != ERR_SUCCESS)
        return error;

    return ERR_SUCCESS;
}

static void protocol_client_request_delete(struct protocol_client_request *request)
{
    sf_mem_free(request);
}

static int32_t create_socket(struct protocol_client_sm_class *self)
{
    int32_t error;
    struct net_socket *socket;

    error = self->net_if->lookup(self->net_if, self->server_name, self->host_ip,
            sizeof(self->host_ip));
    if (error != ERR_SUCCESS) {
        error_log(DEBUG_PRC_SM, ("DNS lookup failed\n"), error);
        return error;
    }

    error = self->net_if->socket(self->net_if, &socket);
    if (error != ERR_SUCCESS) {
        error_log(DEBUG_PRC_SM, ("Socket create failed\n"), error);
        return error;
    }

    self->socket = socket;

    return ERR_SUCCESS;
}


static enum protocol_client_sm_state st_reset(struct protocol_client_sm_class *self)
{
    sys_time_t elapsed_time;

    elapsed_time = system_get_diff_time(self->last_reset);

    /*Delay reset*/
    if (elapsed_time < 1000)
        return SM_RESET;

    self->last_reset = system_get_time();
    if (self->socket) {
        debug_log(DEBUG_PRC_SM, ("%s Close socket\n", self->name));
        self->protocol_client->ops->clear_socket(self->protocol_client);
        self->net_if->close(self->socket);
        self->socket = NULL;
        self->connected = FALSE;
        self->retry_cnt = 0;
    }

    return SM_TRANSMIT;
}

static enum protocol_client_sm_state st_connect(struct protocol_client_sm_class *self)
{
    int32_t error = ERR_WOULD_BLOCK;
    static sys_time_t start_time;
    enum protocol_client_sm_state next_state = SM_CONNECT;

    error = connect_wait(self);
    if (error != ERR_SUCCESS)
        return SM_CONNECT;
    /* Create a new socket if it does not exist */
    if (self->socket == NULL) {
        error = create_socket(self);
        switch (error) {
        case ERR_WOULD_BLOCK:
            return SM_CONNECT;
        case ERR_SUCCESS:
            break;
        default:
            error_log(DEBUG_PRC_SM, ("%s Socket error\n", self->name), error);
            return SM_RESET;
        }
    }

    ASSERT(self->socket);

    /* Connection is already available */
    if (self->connected)
        next_state = SM_TRANSMIT;
    /* Continue the connecting process */
    else {
        if (self->retry_cnt == 0) {
            self->retry_cnt++;
            debug_log(DEBUG_PRC_SM, ("%s Connecting to %s:%d\n", self->name, self->host_ip, self->server_port));
            start_time = system_get_time();
        }

        error = self->net_if->connect(self->socket, self->host_ip, self->server_port);
        switch (error) {

        case ERR_SUCCESS:
            self->connected = TRUE;
            debug_log(DEBUG_PRC_SM, ("%s Connected to host\n", self->name));
            self->protocol_client->ops->set_socket(self->protocol_client, self->socket);
            next_state = SM_TRANSMIT;
            break;

        case ERR_WOULD_BLOCK:
            if (system_get_diff_time(start_time) > self->transmission_config->connect_timeout) {
                if (self->retry_cnt >= self->transmission_config->connect_retries) {
                    next_state = SM_FAILURE;
                } else {
                    debug_log(DEBUG_PRC_SM, ("%s Failed to connect %d. time\n", self->name, self->retry_cnt));
                    next_state = SM_RESET;
                }
            } else {
                next_state = SM_CONNECT;
            }
            break;

        case ERR_NET_TIMEDOUT:
            next_state = SM_RESET;
            error_log(DEBUG_PRC_SM, ("%s connection timeout\n", self->name), error);
            break;

        default:
            error_log(DEBUG_PRC_SM, ("%s Failed to connect\n", self->name), error);
            next_state = SM_RESET;
            break;
        }
    }

    if (error != ERR_WOULD_BLOCK)
        connect_post();

    return next_state;
}

static enum protocol_client_sm_state st_failure(struct protocol_client_sm_class *self)
{
    self->transmit_failures = 0;
    return SM_RESET;
}

static void st_new_transmit(struct protocol_client_sm_class *self)
{
    struct protocol_client_request *request_obj = self->active_request;

    /* If there is no active transfer and the queue is not
     * empty, we prepare the next request_obj.
     **/
    if (bool_get_atomic(self->sm_mutex, &self->abort_transfers_in_progress))
        return;

    if (!request_obj && !cqueue_is_empty(self->transfer_queue)) {
        if ((self->paralell_request_limit == NO_REQUEST_LIMIT)
                || (self->paralell_request_limit > self->sent_request_num)) {
            request_obj = (struct protocol_client_request *) cqueue_pop_head(self->transfer_queue);
            ASSERT(request_obj);
            /*assert(!request_obj->request);*/
            request_obj->start_of_transmit = system_get_time();
        }
    }

    self->active_request = request_obj;
}

static int32_t st_send_request(struct protocol_client_sm_class *self)
{
    int32_t error;
    struct protocol_client_class *protocol_client = self->protocol_client;

    if (!self->active_request)
        return ERR_SUCCESS;

    ASSERT(self->active_request->request);

    debug_log(DEBUG_PRC_SM, ("%s Sending Request\n", self->name));

    error = protocol_client->ops->send_request(protocol_client, self->active_request->request);
    switch (error) {
    case ERR_SUCCESS:
        debug_log(DEBUG_PRC_SM, ("%s Request is sent\n", self->name));
        list_add(&self->active_request->request_list, &self->sent_request_list);
        self->sent_request_num++;
        self->active_request = NULL;
        break;

    case ERR_WOULD_BLOCK:
        if (system_get_diff_time(self->active_request->start_of_transmit) >
                                 self->transmission_config->transmission_timeout) {
            error = ERR_TIMEOUT;
            error_log(DEBUG_PRC_SM, ("%p: Request sending timeout\n", self->name), error);
        }
        break;

    default:
        error_log(DEBUG_PRC_SM, ("%s Sending request failed\n", self->name), error);
        break;
    }

    return error;
}

static int32_t st_recv_response(struct protocol_client_sm_class *self)
{
    int32_t error = ERR_WOULD_BLOCK;
    struct protocol_client_class *protocol_client = self->protocol_client;
    int32_t response_code;
    size_t payload_size;

    error = protocol_client->ops->recv_response(protocol_client, &response_code,
            &self->response.id, self->response.payload,
            MAX_PAYLOAD_SIZE, &payload_size);

    switch (error) {

    case ERR_SUCCESS:
        debug_log(DEBUG_PRC_SM, ("%s Response is received %d\n", self->name, response_code));
        self->response.response_code = response_code;
        self->response.payload_size = payload_size;
        self->response.status = protocol_client->ops->get_response_status(self->protocol_client, response_code);

        break;

    case ERR_WOULD_BLOCK:
        break;

    default:
        self->response.response_code = error;
        self->response.payload_size = 0;
        error_log(DEBUG_PRC_SM, ("%s Receiving response failed\n", self->name), error);
        break;
    }

    return error;
}

static void st_transmit_complete(struct protocol_client_sm_class *self)
{
    struct protocol_client_class *protocol_client = self->protocol_client;
    struct protocol_client_request *completed_request = self->completed_request;
    BOOL complete_request = TRUE;

    debug_log(DEBUG_PRC_SM, ("%s Transmit complete: %dms [%d]\n",
                    self->name,
                    system_get_diff_time(completed_request->start_of_transmit),
                    self->response.response_code));

    self->response.sm = self;
    self->response.max_payload_size = MAX_PAYLOAD_SIZE;
    self->response.request_type = completed_request->type;
    completed_request->clb(completed_request->request, completed_request->request_args, &self->response);

    protocol_client->ops->close(protocol_client);
    if (self->type == SM_ASYNCH && self->completed_request->type == REQUEST_SUBSCRIBE_ASYNCH) {
        complete_request = FALSE;
        if (bool_get_atomic(self->sm_mutex, &self->abort_transfers_in_progress))
            complete_request = TRUE;
        if (self->response.status == FALSE)
            complete_request = TRUE;
    }

    if (!complete_request) {
        self->completed_request = NULL;
    } else {
        if (completed_request) {
            list_del(&completed_request->request_list);
            protocol_client_request_delete(self->completed_request);
            self->completed_request = NULL;
        }
        if (self->sent_request_num)
            self->sent_request_num--;
        ASSERT(self->sent_request_num >= 0);
    }
}

static BOOL pair_request_to_response(struct protocol_client_sm_class *self)
{
    struct protocol_client_request *request;
    struct list_head *pos, *n;
    /*Find a matching request for the respone*/
    list_for_each_safe(pos, n, &self->sent_request_list) {
        request = list_entry(pos, struct protocol_client_request, request_list);
        ASSERT(request);
        debug_log(DEBUG_PRC_SM,
                  ("%s Pairing %d/%d\n", self->name, request->message_id, self->response.id));
        if (request->message_id == self->response.id) {
            self->completed_request = request;
            return TRUE;
        }
    }

    debug_log(DEBUG_PRC_SM, ("%s Could not pair response RC:%d\n",
                                      self->name, self->response.response_code));
    return FALSE;
}

static BOOL check_timeout(struct protocol_client_sm_class *self)
{
    struct protocol_client_request *request;
    struct list_head *pos, *n;
    sys_time_t elapsed_time;

    /*Check the sent queue for timeouts*/
    list_for_each_safe(pos, n, &self->sent_request_list) {
        request = list_entry(pos, struct protocol_client_request, request_list);
        if (self->type == SM_ASYNCH && request->type == REQUEST_SUBSCRIBE_ASYNCH)
            continue;
        ASSERT(request);
        elapsed_time = system_get_diff_time(request->start_of_transmit);
        if (elapsed_time > self->transmission_config->transmission_timeout) {
            error_log(DEBUG_PRC_SM, ("%s Waiting for response timed out elapsed %u\n",
                                     self->name, elapsed_time),
                                     ERR_TIMEOUT);
            self->response.response_code = ERR_TIMEOUT;
            self->response.payload_size = 0;
            self->completed_request = request;
            self->active_request = NULL;
            self->timeouts++;
            return TRUE;
        }
    }

    /*Check the active request for timeout*/
    if (self->active_request) {
        elapsed_time = system_get_diff_time(self->active_request->start_of_transmit);
        if (elapsed_time > self->transmission_config->transmission_timeout) {
            debug_log(DEBUG_PRC_SM,
                      ("%s Sending request timed out elapsed for active request %d\n",
                       self->name, elapsed_time));
            self->response.response_code = ERR_TIMEOUT;
            self->response.payload_size = 0;
            self->completed_request = self->active_request;
            self->active_request = NULL;
            self->timeouts++;
            return TRUE;
        }

    }

    return FALSE;
}

static BOOL abort_transfers(struct protocol_client_sm_class *self)
{
    struct protocol_client_request *request;
    struct list_head *pos, *n;

    if (!bool_get_atomic(self->sm_mutex, &self->abort_transfers_in_progress))
        return FALSE;

    debug_log(DEBUG_PRC_SM, ("%s Abort transfers %d %d\n", self->name,
            self->abort_transfers_in_progress, list_empty(&self->sent_request_list)));

    request = (struct protocol_client_request *) cqueue_pop_head(self->transfer_queue);
    if (request) {
        self->response.response_code = ERR_REQUEST_ABORTED;
        self->response.payload_size = 0;
        self->completed_request = request;
        self->active_request = NULL;
        return TRUE;

    }

    /*Check the sent queue for timeouts*/
    list_for_each_safe(pos, n, &self->sent_request_list) {
        request = list_entry(pos, struct protocol_client_request, request_list);
        ASSERT(request);
        self->response.response_code = ERR_REQUEST_ABORTED;
        self->response.payload_size = 0;
        self->completed_request = request;
        self->active_request = NULL;
        return TRUE;
    }

    /*Check the active request for timeout*/
    if (self->active_request) {
        self->response.response_code = ERR_REQUEST_ABORTED;
        self->response.payload_size = 0;
        self->completed_request = self->active_request;
        self->active_request = NULL;
        return TRUE;

    }

    debug_log(DEBUG_PRC_SM, ("%s All transfers aborted %d\n", self->name));
    bool_set_atomic(self->sm_mutex, &self->abort_transfers_in_progress, FALSE);
    self->abort_done = TRUE;
    return FALSE;
}

/**
 * This handles the sm_transmit state
 * If an error occurs then the state machine goes to SM_TRANSMIT_FAILURE state
 * otherwise it stays in SM_TRANSMIT state
 **/
static enum protocol_client_sm_state st_transmit(struct protocol_client_sm_class *self)
{
    int32_t error;
    BOOL completed = FALSE;
    BOOL completed_with_timeout = FALSE;
    BOOL completed_aborted = FALSE;
    enum protocol_client_sm_state next_state = SM_TRANSMIT;

    st_new_transmit(self);

    next_state = st_connect(self);

    if (next_state == SM_TRANSMIT) {
        error = st_send_request(self);
        switch (error) {
        case ERR_SUCCESS:
            break;
        case ERR_WOULD_BLOCK:
            break;
        default:
            self->transmit_failures++;
            break;
        }

        error = st_recv_response(self);
        switch (error) {
        case ERR_SUCCESS:
        case ERR_OVERFLOW:
            completed = pair_request_to_response(self);
            break;
        case ERR_WOULD_BLOCK:
            break;
        default:
            self->transmit_failures++;
            break;
        }
    }

    if (completed)
        st_transmit_complete(self);

    completed_with_timeout |= check_timeout(self);
    if (completed_with_timeout)
        st_transmit_complete(self);

    completed_aborted = abort_transfers(self);
    if (completed_aborted)
        st_transmit_complete(self);

    if (self->always_disconnect && (completed || completed_with_timeout || completed_aborted))
        next_state = SM_RESET;

    if (self->timeouts > MAX_RESPONSE_TIMEOUT_PER_SESSION) {
        error_log(DEBUG_PRC_SM, ("Too many timeouts: reconnect socket\n"), ERR_FAILURE);
        self->timeouts = 0;
        next_state = SM_RESET;
    }

    if (self->abort_done) {
        self->abort_done = FALSE;
        next_state = SM_RESET;
    }

    if (next_state == SM_CONNECT)
        next_state = SM_TRANSMIT;

    if (next_state == SM_RESET) {
        if (self->socket) {
            debug_log(DEBUG_PRC_SM, ("%s Close socket\n", self->name));
            self->protocol_client->ops->clear_socket(self->protocol_client);
            self->net_if->close(self->socket);
            self->socket = NULL;
            self->connected = FALSE;
            self->retry_cnt = 0;
        }
    }

    return next_state;
}

static void protocol_client_sm_task(void *args)
{
    struct protocol_client_sm_class *self = (struct protocol_client_sm_class *) args;
    static enum protocol_client_sm_state next_state = SM_RESET;

    switch (self->state) {

    case SM_RESET:
        next_state = st_reset(self);
        break;

    case SM_TRANSMIT:
        next_state = st_transmit(self);
        break;

    case SM_FAILURE:
        next_state = st_failure(self);
        break;

    case SM_CONNECT:
    default:
        break;
    }

    if (self->transmit_failures > self->transmission_config->transmission_retries)
        next_state = SM_FAILURE;

    if (self->state != next_state) {
        debug_log(DEBUG_PRC_SM, ("%s moves to %s\n", self->name,
                        (next_state == SM_RESET) ? "Reset" :
                        (next_state == SM_CONNECT) ? "Connecting" :
                        (next_state == SM_TRANSMIT) ? "Transmit state" :
                        (next_state == SM_FAILURE) ? "Failure" :
                        "Unknown state"));
    }

    self->state = next_state;
}

BOOL protocol_client_sm_is_queue_full(struct protocol_client_sm_class *self)
{
    if (cqueue_get_size(self->transfer_queue) == self->transmission_config->protocol_client_sm_queue_size)
        return TRUE;

    return FALSE;
}

int32_t protocol_client_sm_transmit(struct protocol_client_sm_class *self, enum request_type type,
        void *request, void *request_args, int32_t message_id, pc_sm_callback resp_clb)
{
    int32_t error;
    struct protocol_client_request *erequest;

    ASSERT(self);

    debug_log(DEBUG_PRC_SM, ("%s Adding new request, Queue size: %d\n",
              self->name, cqueue_get_size(self->transfer_queue)));

    error = protocol_client_request_new(&erequest);
    if (error != ERR_SUCCESS) {
        error_log(DEBUG_PRC_SM, ("Failed to create client request\n"), error);
        return error;
    }

    erequest->start_of_transmit = 0;
    erequest->request = request;
    erequest->request_args = request_args;
    erequest->clb = resp_clb;
    erequest->type = type;
    erequest->message_id = message_id;
    INIT_LIST_HEAD(&erequest->request_list);
    error = cqueue_push_tail(self->transfer_queue, erequest);
    if (error != ERR_SUCCESS) {
        protocol_client_request_delete(erequest);
        return error;
    }

    return ERR_SUCCESS;
}

void protocol_client_sm_set_identity(struct protocol_client_sm_class *exo,
                                     const char *identity,
                                     const char *passphrase)
{
    if (exo->net_if->set_identity)
        exo->net_if->set_identity(identity, passphrase);
}

int32_t protocol_client_sm_delete(struct protocol_client_sm_class *exo)
{
    /* TODO */
    (void)exo;
    return ERR_SUCCESS;
}

void protocol_client_sm_clear(struct protocol_client_sm_class *self)
{
    bool_set_atomic(self->sm_mutex, &self->abort_transfers_in_progress, TRUE);
}

BOOL protocol_client_sm_is_empty(struct protocol_client_sm_class *self)
{
    return !bool_get_atomic(self->sm_mutex, &self->abort_transfers_in_progress);
}

int32_t protocol_client_sm_new(struct protocol_client_sm_class **exo,
                               const char *name,
                               struct protocol_client_config_class *protocol,
                               const char *server_name,
                               uint16_t server_port)
{
    struct protocol_client_sm_class *e;
    struct periodic_config_class *periodic_config = client_config_get_periodic_config();
    int32_t error;

    /** If this assert fails it most probably means that
     *    - HTTP/COAP support was not built in
     *
     *  It is also possible but less likely that:
     *    - There was critical failure during the HTTP/COAP initalization, like out of memory condition
     * Check your protocol client constructor in protocol_clients/your_client.c
     */
    ASSERT(protocol->exo_api);
    if (!protocol->exo_api) {
        error = ERR_BAD_ARGS;
        goto cleanUp_1;
    }

    error = sf_mem_alloc((void **)&e, sizeof(*e));
    if (error != ERR_SUCCESS)
        goto cleanUp_1;

    error = sf_mem_alloc((void **)&e->server_name, strlen(server_name) + 1);
    if (error != ERR_SUCCESS)
        goto cleanUp_2;

    INIT_LIST_HEAD(&e->sent_request_list);
    e->name = name;
    e->sent_request_num = 0;
    e->paralell_request_limit = protocol->rq_limit;

    e->net_if = protocol->exo_api->net_if;
    e->protocol_client = protocol->exo_api;
    strcpy(e->server_name, server_name);
    e->server_name[strlen(server_name)] = '\0';
    e->server_port = server_port;
    e->connected = FALSE;
    e->socket = NULL;
    e->always_disconnect = FALSE;
    e->state = SM_RESET;
    e->retry_cnt = 0;
    e->active_request = NULL;
    e->type = protocol->stype;
    e->transmit_failures = 0;
    e->transmission_config = client_config_get_transmission_config();
    e->last_reset = 0; /* Start the last reset from the beginning of time */
    e->timeouts = 0;
    e->abort_done = FALSE;

    e->abort_transfers_in_progress = FALSE;

    e->transfer_queue = cqueue_new(e->transmission_config->protocol_client_sm_queue_size);
    if (e->transfer_queue == NULL) {
        error = ERR_NO_MEMORY;
        goto cleanUp_3;
    }

    error = system_mutex_create(&e->sm_mutex);
    if (error != ERR_SUCCESS)
        goto cleanUp_4;

    *exo = e;

    /*TODO This is still temporary, COAP is not yet finalized*/
    protocol_client_sm_set_identity(e, MAKE_STRING(DEVICE_RID), MAKE_STRING(DEVICE_CIK));

    pclb_register(periodic_config->sm_period, protocol_client_sm_task, e, NULL);

    return ERR_SUCCESS;


cleanUp_4: cqueue_free(e->transfer_queue);
cleanUp_3: sf_mem_free(e->server_name);
cleanUp_2: sf_mem_free(e);
cleanUp_1:
    error_log(DEBUG_PRC_SM, ("Failed to create protocol client SM\n"), error);

    return error;
}
