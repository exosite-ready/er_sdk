/**
 * @file exosite_api.c
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
 * This file implements the Exosite API calls
 *
 * This module is responsible for handling all details that are Exosite Server API specific, this includes
 *   - receiving request from the USER through the @ref Exosite_API
 *   - Handling Exosite API and creating abstract requests (exo_request_args) from those calls
 *   - provisioning
 *
 * The requests are then put to queues/dictionaries depending on the request type
 *   - The Subscribe requests are put into a subscribe dictionary
 *   - Every other request is put into a request queue
 *
 * This module implements a state machine, which does the following:
 *   - This state machine handles the provisioning of the device.
 *   - Send the requests from the queues/dictionaries to the Server when the device is in Activated state
 *
 * This module interfaces with the @ref protocol_client_sm, which performs the actual send/recv to the network
 *
 * There are 3 types of state machines that are handled by this module
 *   - TimeSynch: It handles the time synchronization at startup
 *   - Long poll: needed for proper handling of subscribe over HTTP
 *   - Main: Everything else goes into this
 *
 *  Since HTTP is a blocking protocol; one state machine is needed for every subscribe request over http
 *
 **/

#include <stdio.h>
#include <string.h>

#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/mem_check.h>
#include <lib/string_class.h>
#include <lib/pclb.h>
#include <lib/concurrent_queue.h>
#include <lib/dictionary.h>
#include <lib/fault_injection.h>

#include <porting/thread_port.h>
#include <porting/net_port.h>
#include <porting/system_port.h>

#include <system_utils.h>
#include <client_config.h>
#include <server_api.h>
#include <protocol_client_sm.h>
#include <exosite_api_internal.h>
#include <exosite_api.h>

static int32_t activate(struct exosite_class *exo, const char *vendor,
        const char *model, const char *sn);
static void on_activate(int32_t status, const char *cik, struct exo_request_args *args);

enum sm_type {
    SM_MAIN = 0,
    SM_HTTP_LONG_POLL,
    SM_TIME_SYNC
};

static const char *main_sm_name = "Main SM";
static const char *long_poll_sm_name = "Long poll SM";
static const char *time_sync_sm_name = "Time sync SM";

struct sm_list_item {
    enum sm_type type;
    char *name;
    struct protocol_client_sm_class *sm;
    struct sm_list_item *next;
};

enum activate_states  {
        AS_SEND_GET_TIMESTAMP = 1,
        AS_WAIT_TIMESTAMP,
        AS_INIT,
        AS_START,
        AS_PREPARE_ACTIVATE,
        AS_SEND_ACTIVATE,
        AS_WAIT_ACTIVATE,
        AS_ACTIVATED
};

struct exosite_class {
    enum application_protocol_type protocol_type;
    struct server_api_request_ops *server_api;
    struct sm_list_item *sm_head;
    uint16_t numberof_state_machines;
    uint16_t max_numberof_state_machines;
    char cik[CIK_LENGTH + 1];
    char *vendor;
    char *model;
    char *sn;
    CQueue *request_queue;
    struct dictionary_class *subscribe_map;
    struct transmission_config_class *transmission_config;
    enum activate_states activate_state;
    enum activate_states next_state;
    BOOL asynch_state_event;
    system_mutex_t activate_sm_mutex;
    sys_time_t last_activation_time;
    int32_t activate_request_attempts;
    int32_t activate_requests;
    int32_t timestamp;
};

static int32_t select_server_data(struct exosite_class *exo,
                                  enum application_protocol_type protocol_type,
                                  enum security_mode security,
                                  char **server_name,
                                  uint16_t *server_port)
{
    int32_t error;
    struct server_data_class **sd;
    struct server_data_class *server_data;
    int32_t i = 0;

    sd = client_config_get_server_data();
    server_data = sd[i];

    while (server_data != NULL) {
        if (server_data->security == security
                && server_data->protocol_type == protocol_type) {
            char *generated_server_name;
            size_t generated_server_name_size = strlen(
                    server_data->server_name) + strlen(exo->vendor) + 1 + 1;
            error = sf_mem_alloc((void **)&generated_server_name,
                    generated_server_name_size);
            if (error != ERR_SUCCESS)
                return error;
            snprintf(generated_server_name, generated_server_name_size, "%s.%s",
                    exo->vendor, server_data->server_name);
            debug_log(DEBUG_EXOAPI,
                ("Generated server name %s %d/%d\n",
                 generated_server_name, strlen(generated_server_name), generated_server_name_size));
            /*TODO do not use generated server name for COAP, it still does not work
             * *server_name  = server_data->server_name;
             **/
            *server_name = generated_server_name;

            *server_port = server_data->server_port;
            break;
           }
            server_data = sd[++i];
    }
    if (server_data != NULL)
        return ERR_SUCCESS;
    else
        return ERR_NOT_FOUND;
}

static int32_t create_new_sm(struct exosite_class *exo,
                             const char *name,
                             enum application_protocol_type app_type,
                             enum sm_type type)
{
    int32_t error = ERR_SUCCESS;
    struct sm_list_item *new_item;
    struct sm_list_item *item;
    char *server_name = NULL;
    uint16_t server_port = 0;
    struct protocol_client_config_class *protocol_config;
    const char *sm_name;
    enum security_mode security;

    if (exo->max_numberof_state_machines != UNLIMITED_STATE_MACHINES  &&
        exo->numberof_state_machines == CONFIG_NUMBER_OF_STATE_MACHINES)
        return ERR_NO_RESOURCE;

    security = exo->transmission_config->security;
    switch (type) {
    case SM_MAIN:
        sm_name = main_sm_name;
        break;
    case SM_HTTP_LONG_POLL:
        sm_name = long_poll_sm_name;
        break;
    case SM_TIME_SYNC:
        sm_name = time_sync_sm_name;
        security = SECURITY_OFF;
        break;
    default:
        /**
         * A state machine was created with an undefined type
         * Check out create_new_sm calls
         **/
        sm_name = NULL;
        ASSERT(0);
    }

    error = client_config_get_server_api(&exo->server_api, app_type);
    if (error != ERR_SUCCESS)
        return error;

    error = client_config_get_protocol_client_config(&protocol_config, app_type, security);
    if (error != ERR_SUCCESS)
        return error;

    error = select_server_data(exo, app_type, security, &server_name, &server_port);
    if (error != ERR_SUCCESS)
        return error;

    error = sf_mem_alloc((void **)&new_item, sizeof(struct sm_list_item));
    if (error != ERR_SUCCESS)
        return error;

    if (name) {
        error = sf_mem_alloc((void **)&new_item->name, strlen(name) + 1);
        if (error != ERR_SUCCESS) {
            sf_mem_free(new_item);
            return error;
        }

        strcpy(new_item->name, name);
    } else
        new_item->name = NULL;

    error = protocol_client_sm_new(&new_item->sm, sm_name, protocol_config, server_name, server_port);
    if (error != ERR_SUCCESS) {
        if (new_item->name)
            sf_mem_free(new_item->name);

        sf_mem_free(new_item);
        check_memory();
        return error;
    }

    new_item->next = NULL;
    new_item->type = type;

    if (!exo->sm_head)
        exo->sm_head = new_item;
    else {
        item = exo->sm_head;
        while (item->next)
            item = item->next;
        item->next = new_item;
    }

    exo->numberof_state_machines++;
    check_memory();
    return ERR_SUCCESS;
}

static struct protocol_client_sm_class *find_sm(struct exosite_class *exo,
                                                enum sm_type type,
                                                const char *name)
{
    struct sm_list_item *item;

    item = exo->sm_head;
    while (item) {
        if (item->type == type) {
            if (!name)
                return item->sm;
            if (!item->name)
                continue;
            if (!strcmp(name, item->name))
                return item->sm;
        }
        item = item->next;
    }

    return NULL;
}

#define ARG_SIZE(arg)       ((arg)?strlen(arg)+1:0)
#define ARG_CPY(to, from)    ((from)?(strcpy((to), (from))):(to = NULL))

#ifdef TEST_ERSDK
static enum fault_type constructor_fault = FT_NO_ERROR;
static int32_t fault_number;

static int32_t request_args_new_simulate_failure(void)
{
    if (fault_number++ > 5 && constructor_fault == FT_ERROR_AFTER_INIT)
        return ERR_FAILURE;

    if (fault_number % 3 == 0 && constructor_fault == FT_TEMPORARY_ERROR)
        return ERR_FAILURE;

    return ERR_SUCCESS;
}
#endif

static int32_t gmessage_id = 6;
static int32_t exo_request_args_new(struct exo_request_args **prequest, const struct exo_request_args *args)
{
    int32_t error;
    void *new_request = NULL;
    size_t request_buff_size = sizeof(struct exo_request_args);
    struct exo_request_args *new_args;
    char *arg_buffer;
    char **request = (char **)prequest;

#ifdef TEST_ERSDK
    error = request_args_new_simulate_failure();
    if (error != ERR_SUCCESS)
        return error;
#endif

    ASSERT(args);

    /* Calculate size of the needed argument buffer */
    request_buff_size += ARG_SIZE(args->alias);
    request_buff_size += ARG_SIZE(args->value);
    request_buff_size += ARG_SIZE(args->content);
    request_buff_size += ARG_SIZE(args->timeout);
    request_buff_size += ARG_SIZE(args->timestamp);
    request_buff_size += ARG_SIZE(args->vendor);
    request_buff_size += ARG_SIZE(args->model);
    request_buff_size += ARG_SIZE(args->sn);
    request_buff_size += ARG_SIZE(args->id);

    /* Allocate memory for the request structure  and the argument data buffer */
    error = sf_mem_alloc((void **)&new_request, request_buff_size);
    if (error != ERR_SUCCESS)
        return error;

    memset(new_request, 0, request_buff_size);
    new_args = (struct exo_request_args *) new_request;
    arg_buffer = (char *)new_request + sizeof(struct exo_request_args);
    new_args->method = args->method;
    new_args->callback.no_type = args->callback.no_type;
    new_args->clb_params = args->clb_params;
    new_args->subscribe_usr_callback = args->subscribe_usr_callback;
    new_args->exo = args->exo;
    new_args->app_type = args->app_type;
    switch (args->app_type) {
    case APP_PROTO_HTTP:
        new_args->message_id = 1;
        break;
    case APP_PROTO_COAP:
    case APP_PROTO_JSON:
    default:
        new_args->message_id = gmessage_id++;
        break;
    }

    strncpy(new_args->cik, args->cik, 40);
    new_args->cik[40] = '\0';

    /* Set the addresses assigned to the arguments in the
     * argument data buffer
     **/
    if (args->alias) {

        ARG_CPY(arg_buffer, args->alias);
        new_args->alias = arg_buffer;
        arg_buffer += (strlen(new_args->alias) + 1);
    }
    if (args->value) {

        ARG_CPY(arg_buffer, args->value);
        new_args->value = arg_buffer;
        arg_buffer += (strlen(new_args->value) + 1);
    }
    if (args->content) {

        ARG_CPY(arg_buffer, args->content);
        new_args->content = arg_buffer;
        arg_buffer += (strlen(new_args->content) + 1);
    }
    if (args->timeout) {

        ARG_CPY(arg_buffer, args->timeout);
        new_args->timeout = arg_buffer;
        arg_buffer += (strlen(new_args->timeout) + 1);
    }
    if (args->timestamp) {

        ARG_CPY(arg_buffer, args->timestamp);
        new_args->timestamp = arg_buffer;
        arg_buffer += (strlen(new_args->timestamp) + 1);
    }
    if (args->vendor) {
        ARG_CPY(arg_buffer, args->vendor);
        new_args->vendor = arg_buffer;
        arg_buffer += (strlen(new_args->vendor) + 1);
    }
    if (args->model) {
        ARG_CPY(arg_buffer, args->model);
        new_args->model = arg_buffer;
        arg_buffer += (strlen(new_args->model) + 1);
    }
    if (args->sn) {
        ARG_CPY(arg_buffer, args->sn);
        new_args->sn = arg_buffer;
        arg_buffer += (strlen(new_args->sn) + 1);
    }
    if (args->id) {
        ARG_CPY(arg_buffer, args->id);
        new_args->id = arg_buffer;
    }

    *request = new_request;

    check_memory();

    return ERR_SUCCESS;
}

static void exo_request_args_delete(struct exo_request_args *args)
{
    sf_mem_free(args);
}

static void request_delete(struct exo_request_args *args, void *request, enum request_type request_type)
{
    if (!(args->method == EXO_DP_SUBSCRIBE && request_type == REQUEST_SUBSCRIBE_ASYNCH))
        args->server_api->request_delete(request);
    if (args->method != EXO_DP_SUBSCRIBE)
        exo_request_args_delete(args);
}

static void exosite_api_callback(void *req, void *req_args, struct protocol_client_response *response)
{
    struct exo_request_args *args = req_args;
    int32_t status = ERR_FAILURE;

    ASSERT(args->server_api->response_callback);
    status = args->server_api->response_callback(args,
                                        response->payload,
                                        response->payload_size,
                                        response->max_payload_size,
                                        response->response_code);

    if (status == ERR_CIK_NOT_VALID) {
        struct exosite_class *exo = args->exo;

        system_mutex_lock(exo->activate_sm_mutex);
        exo->asynch_state_event = TRUE;
        system_mutex_unlock(exo->activate_sm_mutex);
    }

    switch (args->method) {
    case EXO_DP_WRITE:
        if (args->callback.write)
            args->callback.write(status, args->alias);
        break;
    case EXO_DP_READ:
        if (args->callback.read)
            args->callback.read(status, args->alias,
                    response->payload);
        break;
    case EXO_DP_READ_WRITE:
        /* TODO */
        break;
    case EXO_DP_SUBSCRIBE:
        if (args->callback.subscribe)
            args->callback.subscribe(status, args->app_type, args, response->payload);
        /* TODO */
        break;
    case EXO_PP_ACTIVATE:
        if (args->callback.activate)
            args->callback.activate(status, response->payload, args);
        break;
    case EXO_PP_LIST_CONTENT:
      /* TODO */
        break;
    case EXO_PP_DOWNLOAD_CONTENT:
      /* TODO */
        break;
    case EXO_UP_TIMESTAMP:
        if (args->callback.get_timestamp)
            args->callback.get_timestamp(status, response->payload, args);
      /* TODO */
        break;
    case EXO_UP_IP:
      /* TODO */
        break;
    default:
        ASSERT(0);
        break;
    }

    request_delete(req_args, req, response->request_type);
}

#ifdef TEST_ERSDK
/*
 * With this test function it is possible to emulate the receptions of result
 *
 * With this it is possible to test scenarios where late responses might have an effect

*/
void exosite_sm_inject_result(struct exosite_class *exo, enum exosite_method method, void *callback)
{
    int32_t error;
    struct exo_request_args args;
    struct exo_request_args *new_args;
    struct protocol_client_response response;
    void *request;


    memset(&args, 0, sizeof(args));
    args.method = method;
    args.alias = "Simulation";
    args.value = NULL;
    args.callback.no_type = callback;

    args.app_type = exo->protocol_type;

    response.response_code = 401;

    error = exo_request_args_new(&new_args, &args);
    if (error != ERR_SUCCESS)
        return;

    new_args->server_api = exo->server_api;
    new_args->exo = exo;
    error = exo->server_api->request_new(&request, new_args);
    if (error != ERR_SUCCESS)
        return;

    exosite_api_callback(request, new_args, &response);
}
#endif

struct subscribe_item {
    struct exo_request_args *args;
    BOOL sent;
    BOOL acknowledged;
};

static int32_t exo_subscribe_item_new(struct subscribe_item **new_item, struct exo_request_args *args)
{
    int32_t error;
    struct subscribe_item *item;
    struct exo_request_args *new_args = NULL;

    error = sf_mem_alloc((void **)&item, sizeof(*item));
    if (error != ERR_SUCCESS)
        return error;

    error = exo_request_args_new(&new_args, args);
    if (error != ERR_SUCCESS) {
        sf_mem_free(item);
        return error;
    }

    item->sent = FALSE;
    item->acknowledged = FALSE;
    item->args = new_args;

    *new_item = item;

    return error;
}

static void exo_subscribe_item_delete(struct subscribe_item *item)
{
    if (item != NULL) {
        sf_mem_free(item->args);
        sf_mem_free(item);
    }
}

static int32_t add_to_subscribe_map(struct exosite_class *exo, struct exo_request_args *args)
{
    struct subscribe_item *item = NULL;
    int32_t error;

    debug_log(DEBUG_EXOAPI, ("Add to transmit subscribe map\n"));

    args->app_type = exo->protocol_type;

    error = exo_subscribe_item_new(&item, args);
    if (error != ERR_SUCCESS)
        return error;

    error = dictionary_add(exo->subscribe_map, args->alias, item);
    if (error != ERR_SUCCESS) {
        exo_subscribe_item_delete(item);
        return error;
    }

    return error;
}

/**
 * Takes an exo_request_args structure, copies it and pushes it on the request queue
 *
 * @param[in] exo    The exosite object; It gets the queue from there
 * @param[in] args   The object to be pushed on the queueu
 *
 * If it fails it frees the copied object
 * This function can fail if
 *    - there is not enough memory to copy or to push onto the queue
 *    - if the maximum queue size is reached
 *
 * @return ERR_SUCCESS on success ERR_* otherwise
 **/
static int32_t request_queue_push(struct exosite_class *exo, struct exo_request_args *args)
{
    struct exo_request_args *new_args = NULL;
    int32_t error;

    debug_log(DEBUG_EXOAPI, ("Add to transmit queue; method: %d (queue size: %d)\n",
              args->method, cqueue_get_size(exo->request_queue)));

    args->app_type = exo->protocol_type;

    error = exo_request_args_new(&new_args, args);
    if (error != ERR_SUCCESS)
        return error;

    error = cqueue_push_tail(exo->request_queue, new_args);
    if (error != ERR_SUCCESS) {
        exo_request_args_delete(new_args);
    }

    return error;
}

/**
 * Selects a State Machine for the exo_request_args provided and pushes this args object to it
 * without copying
 **/
static int32_t pcsm_transmit_qeueue_push_no_copy(struct exosite_class *exo, struct exo_request_args *args)
{
    struct protocol_client_sm_class *sm = NULL;
    int32_t error = ERR_FAILURE;
    enum request_type type = REQUEST_NORMAL;

    args->exo = (void *)exo;
    switch (args->method) {
    case EXO_DP_WRITE:
    case EXO_DP_READ:
    case EXO_DP_READ_WRITE:
    case EXO_PP_ACTIVATE:
    case EXO_PP_LIST_CONTENT:
    case EXO_PP_DOWNLOAD_CONTENT:
    case EXO_UP_IP:
        sm = find_sm(exo, SM_MAIN, NULL);
        if (!sm) {
            error = create_new_sm(exo, NULL, exo->protocol_type, SM_MAIN);
            if (error) {
                error_log(DEBUG_EXOAPI, ("Failed to create Main sm\n"), error);
                return error;
            }
            sm = find_sm(exo, SM_MAIN, NULL);
            ASSERT(sm);
        }
        type = REQUEST_NORMAL;
        break;
    case EXO_UP_TIMESTAMP:
        sm = find_sm(exo, SM_TIME_SYNC, NULL);
        if (!sm) {
            error = create_new_sm(exo, args->alias, exo->protocol_type, SM_TIME_SYNC);
            if (error != ERR_SUCCESS) {
                error_log(DEBUG_EXOAPI, ("Failed to create time sync SM\n"), error);
                return error;
            }
            sm = find_sm(exo, SM_TIME_SYNC, NULL);
            ASSERT(sm);
        }
        type = REQUEST_NORMAL;
        break;
    case EXO_DP_SUBSCRIBE:

        /* In case of HTTP, this action is implemented
         * with long-poll method. This method need a new
         * state machine instance for each data source.
         **/
        if (exo->protocol_type == APP_PROTO_HTTP) {
            sm = find_sm(exo, SM_HTTP_LONG_POLL, args->alias);
            if (!sm) {
                error = create_new_sm(exo, args->alias, APP_PROTO_HTTP,
                        SM_HTTP_LONG_POLL);
                if (error != ERR_SUCCESS) {
                    error_log(DEBUG_EXOAPI, ("Failed to create new_sm\n"), error);
                    return error;
                }
                sm = find_sm(exo, SM_HTTP_LONG_POLL, args->alias);
                ASSERT(sm);
            }
        } else {
            sm = find_sm(exo, SM_MAIN, NULL);
            if (!sm) {
                error = create_new_sm(exo, NULL, exo->protocol_type, SM_MAIN);
                if (error) {
                    error_log(DEBUG_EXOAPI, ("Failed to create Main sm\n"), error);
                    return error;
                }
                sm = find_sm(exo, SM_MAIN, NULL);
                ASSERT(sm);
            }
        }
        if (exo->protocol_type == APP_PROTO_COAP)
            type = REQUEST_SUBSCRIBE_ASYNCH;
        else
            type = REQUEST_SUBSCRIBE;

        break;

    default:
        ASSERT(0);
        break;
    }

    ASSERT(sm);
    if (sm) {
        void *request;

        /** Server API might not be set in the exosite object before
         * pcsm_transmit_qeueue_push_no_copy is called
         **/
        args->server_api = exo->server_api;
        /* Update the cik, it might change any time*/
        memcpy(args->cik, exo->cik, 40);
        args->cik[40] = '\0';

        if (!protocol_client_sm_is_queue_full(sm)) {
            error = exo->server_api->request_new(&request, args);
            if (error != ERR_SUCCESS)
                return error;

            error = protocol_client_sm_transmit(sm,
                                                type,
                                                request,
                                                args,
                                                args->message_id,
                                                exosite_api_callback);
            if (error != ERR_SUCCESS) {
                exo->server_api->request_delete(request);
                return error;
            }
        }
    }

    return error;
}

static int32_t pcsm_transmit_queue_push(struct exosite_class *exo, const struct exo_request_args *args)
{
    int32_t error;
    struct exo_request_args *new_args;

    error = exo_request_args_new(&new_args, args);
    if (error != ERR_SUCCESS)
        return error;

    error = pcsm_transmit_qeueue_push_no_copy(exo, new_args);
    if (error != ERR_SUCCESS) {
        exo_request_args_delete(new_args);
    }

    return error;
}

static int32_t api_get_cik(char *cik, size_t len)
{
    int32_t error;

    error = system_get_cik(cik, len);
    if (error != ERR_SUCCESS)
        return error;

    if (!system_is_cik_format_valid(cik))
        return ERR_CIK_NOT_VALID;

    return ERR_SUCCESS;
}

static int32_t activate(struct exosite_class *exo, const char *vendor,
        const char *model, const char *sn)
{
    int32_t error;
    struct exo_request_args args;

    memset(&args, 0, sizeof(args));
    args.method = EXO_PP_ACTIVATE;
    args.vendor = vendor;
    args.model = model;
    args.sn = sn;
    args.callback.activate = on_activate;
    args.value = NULL;

    debug_log(DEBUG_NET, ("Send device activate command\n  Vendor: %s\n  Model %s\n  SN: %s\n", vendor, model, sn));

    error = pcsm_transmit_queue_push(exo, &args);

    return error;
}

static void on_timestamp(int32_t status, const char *timestr, struct exo_request_args *args)
{
    struct exosite_class *exo = args->exo;

    debug_log(DEBUG_NET, ("Time stamp received from server\n"));

    if (status == ERR_SUCCESS) {
        sys_time_t timestamp;
        char *pEnd;

        timestamp = (sys_time_t)(strtol(timestr, &pEnd, 10)); /* TODO zr: Fix it It is not type-safe */
        if (pEnd == timestr) {
            debug_log(DEBUG_NET, ("Could not convert\n"));
            exo->next_state = AS_SEND_GET_TIMESTAMP;
        } else {
            rtc_set(timestamp);
            debug_log(DEBUG_NET, ("Done syncing time\n"));
            exo->next_state = AS_INIT;
        }
    } else {
        debug_log(DEBUG_NET, ("Error syncing time\n"));
        exo->next_state = AS_SEND_GET_TIMESTAMP;
    }
}

static int32_t get_timestamp(struct exosite_class *exo)
{
    int32_t error;
    struct exo_request_args args;

    memset(&args, 0, sizeof(args));
    args.method = EXO_UP_TIMESTAMP;
    args.callback.get_timestamp = on_timestamp;
    args.value = NULL;

    debug_log(DEBUG_NET, ("Getting time stamp from server\n"));

    error = pcsm_transmit_queue_push(exo, &args);

    return error;
}

static BOOL should_delay_activate(struct exosite_class *exo)
{
#if ACTIVATION_RETRY_TYPE == 1 /* Linear delay */
    if (system_get_diff_time(exo->last_activation_time) > ACTIVATION_DELAY) {
        exo->last_activation_time = system_get_time();
        return FALSE;
    }

    return TRUE;
#elif ACTIVATION_RETRY_TYPE == 2 /* Exponential backoff */
    (void)exo;
    exo->activate_requests++;
    if (exo->activate_requests >> exo->activate_request_attempts  == 0) {

        return TRUE;
    }

    exo->activate_request_attempts++;
    return FALSE;
#else
    (void)exo;
    return FALSE;
#endif
}

/* The activation state machine */

static void print_state_change(struct exosite_class *exo)
{
    if (exo->next_state != exo->activate_state) {
        debug_log(DEBUG_EXOAPI, ("Exosite SM moves to %s\n",
                        (exo->next_state == AS_SEND_GET_TIMESTAMP) ? "Send get timestamp" :
                        (exo->next_state == AS_WAIT_TIMESTAMP) ? "Wait timestamp" :
                        (exo->next_state == AS_INIT) ? "Init" :
                        (exo->next_state == AS_START) ? "Start" :
                        (exo->next_state == AS_PREPARE_ACTIVATE) ? "Prepare activate" :
                        (exo->next_state == AS_SEND_ACTIVATE) ? "Send activate" :
                        (exo->next_state == AS_WAIT_ACTIVATE) ? "Wait activate" :
                        (exo->next_state == AS_ACTIVATED) ? "Activated" :
                        "Unknown state"));
    }
}

static void on_activate(int32_t status, const char *cik, struct exo_request_args *args)
{
    struct exosite_class *exo = args->exo;
    int32_t error = ERR_SUCCESS;

    ASSERT(exo);

    if (status == ERR_SUCCESS) {
        memcpy(exo->cik, cik, sizeof(exo->cik));
        exo->cik[sizeof(exo->cik) - 1] = '\0';
        info_log(DEBUG_EXOAPI, ("Activate request returned cik %s\n", exo->cik));

        exo->activate_request_attempts = 0;
        exo->activate_requests = 0;

        if (system_is_cik_format_valid(exo->cik)) {
            exo->next_state = AS_ACTIVATED;
            error = system_set_cik(exo->cik);
            if (error != ERR_SUCCESS)
                error_log(DEBUG_NET, ("ERROR: CIK can not be saved!\n"), error);

            system_mutex_lock(exo->activate_sm_mutex);
            exo->asynch_state_event = FALSE;
            system_mutex_unlock(exo->activate_sm_mutex);
        } else {
            /* Do not erase CIK in any case; this is one of the requirements of the activation method */
            /* memset(exo->cik, '\0', sizeof(exo->cik)); */
            error_log(DEBUG_NET, ("ERROR: Invalid CIK format\n"), ERR_CIK_NOT_VALID);
            exo->next_state = AS_PREPARE_ACTIVATE;
            ASSERT(0);
        }
    } else {
        error_log(DEBUG_EXOAPI, ("Activate request returned error\n"), status);
        exo->next_state = AS_PREPARE_ACTIVATE;
    }
}

static void activate_sm(void *obj)
{
    int32_t error;
    struct exosite_class *exo = obj;
    struct exo_request_args *args;
    struct sm_list_item *item;
    BOOL device_activated;
    static enum activate_states old_state;

    print_state_change(exo);
    old_state = exo->activate_state;
    exo->activate_state = exo->next_state;


    switch (exo->activate_state) {
    case AS_SEND_GET_TIMESTAMP:
        error = get_timestamp(exo);
        if (error == ERR_SUCCESS)
            exo->next_state = AS_WAIT_TIMESTAMP;
        break;
    case AS_WAIT_TIMESTAMP:
        break;
    case AS_INIT:
        strcpy(exo->cik, MAKE_STRING(DEVICE_CIK));
        device_activated = system_is_cik_format_valid(exo->cik);
        if (device_activated) {
            exo->next_state = AS_ACTIVATED;
        } else {
            exo->next_state = AS_START;
            memset(exo->cik, '\0', sizeof(*exo->cik));
        }
        break;
    case AS_START:
        error = api_get_cik(exo->cik, sizeof(exo->cik));
        if (error == ERR_SUCCESS)
            exo->next_state = AS_ACTIVATED;
        else
            exo->next_state = AS_PREPARE_ACTIVATE;
        break;
    case AS_PREPARE_ACTIVATE:
        /**
         *  - Only send activate requests at fixed intervals (or exponential backoff)
         *  - Clear the underlying protocol client SMs, so that we don't get a late
         *    response (this cannot be achieved 100% for HTTP)
         **/

        if (should_delay_activate(exo))
            break;
        debug_log(DEBUG_EXOAPI, ("Clear SMs\n"));
        for (item = exo->sm_head; item; item = item->next)
            protocol_client_sm_clear(item->sm);
        exo->next_state = AS_SEND_ACTIVATE;
        break;
    case AS_SEND_ACTIVATE:
    {
        BOOL clear = TRUE;

        if (old_state != exo->activate_state) {
            debug_log(DEBUG_EXOAPI, ("Wait for clear\n"));
        }
        for (item = exo->sm_head; item; item = item->next)
            clear &= protocol_client_sm_is_empty(item->sm);

        if (clear)
            debug_log(DEBUG_EXOAPI, ("All SM are clear\n"));
        else
            break;

        error = activate(exo, exo->vendor, exo->model, exo->sn);
        if (error == ERR_SUCCESS)
            exo->next_state = AS_WAIT_ACTIVATE;
        break;
    }
    case AS_WAIT_ACTIVATE:
        break;
    case AS_ACTIVATED:
    {
        struct dict_iterator *it;
        struct subscribe_item *subs_item;

        system_mutex_lock(exo->activate_sm_mutex);
        if (exo->asynch_state_event == TRUE) {
            exo->asynch_state_event = FALSE;
            exo->next_state = AS_PREPARE_ACTIVATE;
            debug_log(DEBUG_EXOAPI, ("Response's error code forces reactivation\n"));
            system_mutex_unlock(exo->activate_sm_mutex);
            break;
        }
        system_mutex_unlock(exo->activate_sm_mutex);

        for (it = dictionary_get_iterator(exo->subscribe_map);  it != NULL; it = dictionary_iterate(it)) {
            subs_item = dictionary_get_value(it);
            if (subs_item->sent != TRUE && subs_item->acknowledged != TRUE) {
                trace_log(DEBUG_EXOAPI, ("Pushing not acked subscribe request\n"));
                error = pcsm_transmit_qeueue_push_no_copy(exo, subs_item->args);
                if (error == ERR_SUCCESS) {
                    subs_item->sent = TRUE;
                } else {
                    trace_log(DEBUG_EXOAPI, ("Error sending subscribe\n"));
                }

            }
        }

        if (old_state != exo->activate_state) {
            info_log(DEBUG_EXOAPI, ("Device activated %s\n", exo->cik));
        }

        args = cqueue_peek_head(exo->request_queue);
        if (args) {
            error = pcsm_transmit_qeueue_push_no_copy(exo, args);
            if (error == ERR_SUCCESS) {
                args = cqueue_pop_head(exo->request_queue);
                ASSERT(args);
            }
        }
    }
        break;

    default:
        break;
    }

    return;
}

int32_t exosite_read(struct exosite_class *exo, const char *alias,
        exosite_on_read on_read)
{
    int32_t error;
    struct exo_request_args args;

    memset(&args, 0, sizeof(args));
    args.method = EXO_DP_READ;
    args.alias = alias;
    args.value = NULL;
    args.callback.read = on_read;

    error = request_queue_push(exo, &args);

    return error;
}

int32_t exosite_write(struct exosite_class *exo, const char *alias,
        const char *value, exosite_on_write on_write)
{
    int32_t error;
    struct exo_request_args args;

    memset(&args, 0, sizeof(args));
    args.method = EXO_DP_WRITE;
    args.alias = alias;
    args.value = value;
    args.callback.write = on_write;

    error = request_queue_push(exo, &args);

    return error;
}

static void on_subscribe(int32_t status,
                         enum application_protocol_type app_type,
                         struct exo_request_args *request_args,
                         const char *value)
{
    exosite_on_change user_clb = (exosite_on_change) request_args->subscribe_usr_callback;
    struct exosite_class *exo = request_args->exo;
    struct subscribe_item *item;

    (void)app_type;

    if (status != ERR_UNCHANGED)
        if (user_clb)
            user_clb(status, request_args->alias, value);

    item = dictionary_lookup(exo->subscribe_map, request_args->alias);
    if (item) {
        if (status != ERR_SUCCESS) {
            item->sent = FALSE;
            item->acknowledged = FALSE;
        } else {
            item->acknowledged = TRUE;
            if (app_type == APP_PROTO_HTTP) {
                item->sent = FALSE;
                item->acknowledged = FALSE;
            }
        }
    }

    /*If there was a subscribe timeout, then put back the request here*/
}

#define TIMEOUT_STR_SIZE 16
int32_t exosite_subscribe(struct exosite_class *exo,
                          const char *alias,
                          int32_t since,
                          exosite_on_change on_change)
{
    int32_t error;
    struct exo_request_args args;
    char timeout_str[TIMEOUT_STR_SIZE];
    int32_t n;

    /*
     * Use transmission timeout - 15 sec as long poll timeout so that the response can surely arrive; even if
     * the SSL connect takes very long
     **/
    n = snprintf(timeout_str, TIMEOUT_STR_SIZE, "%d", (int)(exo->transmission_config->transmission_timeout - 15000));
    if (n == TIMEOUT_STR_SIZE) {
        ASSERT(0);
        return ERR_FAILURE;
    }

    (void)since;
    memset(&args, 0, sizeof(args));
    args.method = EXO_DP_SUBSCRIBE;
    args.alias = alias;
    args.timeout = timeout_str;
    args.subscribe_usr_callback = on_change;
    args.callback.subscribe = on_subscribe;

    error = add_to_subscribe_map(exo, &args);

    return error;
}

enum exosite_device_status exosite_get_status(struct exosite_class *exo)
{
    enum exosite_device_status status;

    switch (exo->activate_state) {
    case AS_ACTIVATED:
        status = DEVICE_STATUS_ACTIVATED;
        break;

    case AS_SEND_GET_TIMESTAMP:
    case AS_WAIT_TIMESTAMP:
    case AS_INIT:
    case AS_START:
    case AS_PREPARE_ACTIVATE:
    case AS_SEND_ACTIVATE:
    case AS_WAIT_ACTIVATE:
    default:
        status = DEVICE_STATUS_NOT_ACTIVATED;
        break;
    }

    return status;
}

inline void exosite_delay_and_poll(struct exosite_class *exo, sys_time_t ms)
{
    (void)exo;
    system_delay_and_poll(ms);
}

inline void exosite_poll(struct exosite_class *exo)
{
    (void)exo;
    system_poll(0);
}

int32_t exosite_new(struct exosite_class **exo,
                    const char *vendor,
                    const char *model,
                    const char *optional_sn,
                    enum application_protocol_type app_type)
{
    struct exosite_class *e;
    int32_t error;
    const char *sn;
    struct periodic_config_class *periodic_config;

    ASSERT(vendor);
    ASSERT(model);

    if (optional_sn)
        sn = optional_sn;
    else
        sn = system_get_sn();

    error = sf_mem_alloc((void **)&e, sizeof(*e));
    if (error != ERR_SUCCESS)
        goto cleanup1;

    sf_memfill(e, 0, sizeof(*e));
    e->sm_head = NULL;
    e->numberof_state_machines = 0;
    e->max_numberof_state_machines = CONFIG_NUMBER_OF_STATE_MACHINES;
    error = sf_mem_alloc((void **)&e->vendor, strlen(vendor) + 1);
    if (error != ERR_SUCCESS)
        goto cleanup2;

    error = sf_mem_alloc((void **)&e->model, strlen(model) + 1);
    if (error != ERR_SUCCESS)
        goto cleanup3;

    error = sf_mem_alloc((void **)&e->sn, strlen(sn) + 1);
    if (error != ERR_SUCCESS)
        goto cleanup4;

    e->transmission_config = client_config_get_transmission_config();

    /**
     * It copies the model/vendor/sn parameters into internal variables
     * in order to the caller be able ot free the original memory area
     * allocated for these parameters.
     */
    strcpy(e->sn, sn);
    strcpy(e->vendor, vendor);
    strcpy(e->model, model);

    e->request_queue = cqueue_new(e->transmission_config->max_request_queue_size);
    if (e->request_queue == NULL)
        goto cleanup5;

    error = dictionary_new(&e->subscribe_map);
    if (error != ERR_SUCCESS)
        goto cleanup6;

    error = system_mutex_create(&e->activate_sm_mutex);
    if (error != ERR_SUCCESS)
        goto cleanup7;

    e->protocol_type = app_type;
    e->next_state = AS_SEND_GET_TIMESTAMP;
    e->asynch_state_event = FALSE;
    e->activate_request_attempts = 0;
    e->activate_requests = 0;

    periodic_config = client_config_get_periodic_config();
    error = pclb_register(periodic_config->sm_period, activate_sm, e, NULL);
    if (error != ERR_SUCCESS)
        goto cleanup8;

    *exo = e;

    return ERR_SUCCESS;

cleanup8:
    system_mutex_destroy(e->activate_sm_mutex);
cleanup7:
    /*TODO free dictionary*/
cleanup6:
    cqueue_free(e->request_queue);
cleanup5:
    sf_mem_free(e->sn);
cleanup4:
    sf_mem_free(e->model);
cleanup3:
    sf_mem_free(e->vendor);
cleanup2:
    sf_mem_free(e);
cleanup1:

    return ERR_SUCCESS;
}

int32_t exosite_sdk_register_periodic_fn(void (*platform_periodic_fn)(void *data), timer_data_type period)
{
    if (platform_periodic_fn != NULL)
        return pclb_register(period, platform_periodic_fn, NULL, NULL);
    else
        return ERR_SUCCESS;
}
