/*
 * configurator_server.c
 *
 *  Created on: Dec 8, 2015
 *      Author: Zoltan Ribi
 */

#include <stdio.h>
#include <stdlib.h>
#include <sdk_config.h>
#include <string.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/pclb.h>
#include <lib/dictionary.h>
#include <lib/string_class.h>
#include <system_utils.h>
#include <sdk_config.h>
#include <porting/net_port.h>
#include <configurator_api.h>
#include <third_party/http-parser-master/http_parser.h>
#include <lib/pmatch.h>
#include <porting/system_port.h>
#include <lib/sf_malloc.h>

#define CONFIGURATOR_BUFFER_SIZE               1024

extern char configurator_html_opener[];
extern char configurator_wifi_setting_form[];
extern char configurator_error_notification[];
extern char configurator_success_notification[];
extern char configurator_html_closer[];

enum page_type {
    PAGE_BLANK = 0,
    PAGE_MAIN,
    PAGE_ERROR,
    PAGE_SUCCESS
};

struct client_socket {
    struct net_socket *socket;
    enum {
        CLOSED = 0,         /*<! The socket is closed */
        OPENED,             /*<! The socket is opened and waiting for action */
        DISCONNECTED,       /*<! The client closed the socket */
        RECV_IN_PROGRESS,   /*<! Message receiving is in progress on the socket */
        MESSAGE_RECVD,      /*<! New message received */
        SEND_IN_PROGRESS,   /*<! Response sending is in progress on the socket */
        RESPONSE_SENT,      /*<! Response has been sent */
        HTTP_ERROR,         /*<! HTTP parsing error (invalid packet) */
        SOCKET_ERROR        /*<! Socket error */
    } status;

    http_parser parser;
    struct string_class *header_field;
    struct string_class *header_value;
    struct string_class *response;
    size_t sent;
    struct dictionary_class *header_fields;
};

static struct _configurator_server {
    struct net_dev_operations *net_if;
    struct net_socket *server_socket;
    struct client_socket client_socket[MAX_CONFIGURATOR_CONNECTION];
    size_t connections;
    char input_buff[CONFIGURATOR_BUFFER_SIZE];
    http_parser_settings parser_settings;
    char ip[16];
    uint16_t port;
    struct wifi_settings wifi;
    BOOL cfg_changed;
    int32_t (*clb_set)(struct wifi_settings *);
    char *sn;
} cfg_server;


struct header_field_filter {
    const char *header_field;
    const char *pattern;
    void (*clb)(size_t num_of_cap, struct capture *cap);
};

static void clb_set_ssid(size_t num_of_cap, struct capture *cap);
static void clb_set_security(size_t num_of_cap, struct capture *cap);
static void clb_set_passpharas(size_t num_of_cap, struct capture *cap);

static struct header_field_filter fields_of_interest[] = {
    {"URL", "network_settings.*[%?&]ssid=([^&]*)", clb_set_ssid},
    {"URL", "network_settings.*[%?&]security=([^&]*)", clb_set_security},
    {"URL", "network_settings.*[%?&]passphrase=([^&]*)", clb_set_passpharas}
};

static void clb_set_ssid(size_t num_of_cap, struct capture *cap)
{
    ASSERT(num_of_cap <= 1);

    if (num_of_cap != 0) {
        ASSERT(cap->len+1 <= sizeof(cfg_server.wifi.ssid));
        strncpy(cfg_server.wifi.ssid, cap->init, cap->len+1);
        debug_log(DEBUG_CONFIGURATOR, ("SSID: %s\n", cfg_server.wifi.ssid));
        cfg_server.cfg_changed = TRUE;
    }
}

static void clb_set_security(size_t num_of_cap, struct capture *cap)
{
    ASSERT(num_of_cap <= 1);

    if (num_of_cap != 0) {
        if (!strncmp("open", cap->init, cap->len))
            cfg_server.wifi.security = OPEN;
        else if (!strncmp("wpa2", cap->init, cap->len))
            cfg_server.wifi.security = WPA2;
        else {
            ASSERT(0);
            error_log(DEBUG_CONFIGURATOR, ("Unknown Security mode\n"), ERR_NOT_IMPLEMENTED);
        }

        debug_log(DEBUG_CONFIGURATOR, ("Security: %s\n",
                                      (cfg_server.wifi.security == WPA2) ? "WPA2" :
                                      (cfg_server.wifi.security == OPEN) ? "OPEN" :
                                       "Unknown"));
        cfg_server.cfg_changed = TRUE;
    }
}

static void clb_set_passpharas(size_t num_of_cap, struct capture *cap)
{
    ASSERT(num_of_cap <= 1);

    if (num_of_cap != 0) {
        ASSERT(cap->len+1 <= sizeof(cfg_server.wifi.passpharase));
        strncpy(cfg_server.wifi.passpharase, cap->init, cap->len+1);
        debug_log(DEBUG_CONFIGURATOR, ("Passphrase: %s\n", cfg_server.wifi.passpharase));
        cfg_server.cfg_changed = TRUE;
    }
}

static void message_cleanup(struct client_socket *client)
{
    string_delete(client->header_field);
    client->header_field = NULL;

    string_delete(client->header_value);
    client->header_value = NULL;

    string_delete(client->response);
    client->response = NULL;

    client->status = OPENED;
    client->sent = 0;
}

static int32_t process_header_field(const char *field, const char *value)
{
    size_t i;

    ASSERT(field != NULL);
    ASSERT(value != NULL);

    debug_log(DEBUG_CONFIGURATOR, ("%s: %s\n", field, value));

    for (i = 0; i < sizeof(fields_of_interest) / sizeof(struct header_field_filter); i++) {
        if (!strcmp(field, fields_of_interest[i].header_field)) {

            size_t num_of_cap = 0;
            struct capture captures[10];

            str_find(value,
                     strlen(value),
                     fields_of_interest[i].pattern,
                     strlen(fields_of_interest[i].pattern),
                     FALSE,
                     0,
                     &num_of_cap,
                     captures,
                     10);

            fields_of_interest[i].clb(num_of_cap, captures);
        }
    }

    return ERR_SUCCESS;
}

static int32_t set_response(struct client_socket *client, enum page_type page)
{
    int32_t status;
    char response_header[128];
    struct string_class *response_payload;

    (void)client;
    (void)page;

    if (client->response != NULL) {
        ASSERT(0);
        string_delete(client->response);
    }
    client->sent = 0;

    /** 1. Compose the response payload (HTML page) */
    /*----------------------------------------------*/
    status = string_new(&response_payload, NULL);
    if (status != ERR_SUCCESS)
        goto cleanUp1;

    status = string_catc(response_payload, configurator_html_opener);
    if (status != ERR_SUCCESS)
        goto cleanUp2;

    status = string_catc(response_payload, cfg_server.sn);
    if (status != ERR_SUCCESS)
        goto cleanUp2;

    if (page != PAGE_BLANK) {

        status = string_catc(response_payload, configurator_wifi_setting_form);
        if (status != ERR_SUCCESS)
            goto cleanUp2;

        switch (page) {
        case PAGE_MAIN:
            break;
        case PAGE_ERROR:
            status = string_catc(response_payload, configurator_error_notification);
            if (status != ERR_SUCCESS)
                goto cleanUp2;
            break;
        case PAGE_SUCCESS:
            status = string_catc(response_payload, configurator_success_notification);
            if (status != ERR_SUCCESS)
                goto cleanUp2;
            break;
        case PAGE_BLANK:
        default:
            break;
        }
    }

    status = string_catc(response_payload, configurator_html_closer);
    if (status != ERR_SUCCESS)
        goto cleanUp2;

    /** 2. Compose the response (HTTP framing + Payload) */
    /*---------------------------------------------------*/
    sprintf(response_header,
            "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n",
            string_get_size(response_payload));

    status = string_new(&client->response, response_header);
    if (status != ERR_SUCCESS)
        goto cleanUp2;

    status = string_cat(client->response, response_payload);

cleanUp2:
    string_delete(response_payload);
cleanUp1:
    if (status != ERR_SUCCESS) {
        string_delete(client->response);
        client->response = NULL;
    }
    return status;
}

static int on_message_begin(http_parser *self)
{
    struct client_socket *client = (struct client_socket *)self->data;

    message_cleanup(client);

    client->status = RECV_IN_PROGRESS;
    debug_log(DEBUG_CONFIGURATOR, ("***MESSAGE BEGIN***\n"));

    return 0;
}

static int on_headers_complete(http_parser *self)
{
    int32_t status = ERR_SUCCESS;
    struct client_socket *client = (struct client_socket *)self->data;

    if (client->status == HTTP_ERROR)
        return 0;

    if ((client->header_field != NULL) || (client->header_value != NULL)) {
        status = process_header_field(string_get_cstring(client->header_field),
                                      string_get_cstring(client->header_value));
        string_delete(client->header_field);
        client->header_field = NULL;
        string_delete(client->header_value);
        client->header_value = NULL;
    }

    if (status != ERR_SUCCESS)
        client->status = HTTP_ERROR;

    debug_log(DEBUG_CONFIGURATOR, ("***HEADERS COMPLETE***\n"));
    return 0;
}

static int on_message_complete(http_parser *self)
{
    struct client_socket *client = (struct client_socket *)self->data;

    debug_log(DEBUG_CONFIGURATOR, ("***MESSAGE COMPLETE***\n"));

    client->status = MESSAGE_RECVD;

    return 0;
}

static int on_url(http_parser *self, const char *at, size_t length)
{
    int32_t status = ERR_SUCCESS;
    struct client_socket *client = (struct client_socket *)self->data;
    struct string_class *url;

    /* If some error occurred before */
    if (client->status == HTTP_ERROR)
        return 0;

    status = string_new_const(&url, at, length);
    if (status == ERR_SUCCESS)
        status = process_header_field("URL", string_get_cstring(url));

    if (status != ERR_SUCCESS) {
        ASSERT(0);
        error_log(DEBUG_CONFIGURATOR, ("HTTP parsing failure\n"), status);
        client->status = HTTP_ERROR;
    }

    string_delete(url);

    return 0;
}

static int on_header_field(http_parser *self, const char *at, size_t length)
{
    int32_t status = ERR_SUCCESS;
    struct client_socket *client = (struct client_socket *)self->data;

    /* If some error occurred before */
    if (client->status == HTTP_ERROR)
        return 0;

    /* If header_value is not NULL the previous field-value pair
     * is complete. In this case, we add the pair to the dictionary
     */
    if (client->header_value != NULL) {
        status = process_header_field(string_get_cstring(client->header_field),
                                      string_get_cstring(client->header_value));
        string_delete(client->header_field);
        client->header_field = NULL;
        string_delete(client->header_value);
        client->header_value = NULL;

        if (status != ERR_SUCCESS) {
            client->status = HTTP_ERROR;
            return 0;
        }
    }

    /* First chunk of a new header field */
    if (client->header_field == NULL) {
        status = string_new(&client->header_field, "");
        ASSERT(status == ERR_SUCCESS);
    }

    if (status == ERR_SUCCESS)
        status = string_catcn(client->header_field, at, length);

    if (status != ERR_SUCCESS) {
        ASSERT(0);
        error_log(DEBUG_CONFIGURATOR, ("HTTP parsing failure\n"), status);
        client->status = HTTP_ERROR;
    }

    return 0;
}

static int on_header_value(http_parser *self, const char *at, size_t length)
{
    int32_t status = ERR_SUCCESS;
    struct client_socket *client = (struct client_socket *)self->data;

    ASSERT(client->header_field != NULL);

    /* If some error occurred before */
    if (client->status == HTTP_ERROR)
        return 0;

    /* First chunk of a new header field */
    if (client->header_value == NULL) {
        status = string_new(&client->header_value, "");
        ASSERT(status == ERR_SUCCESS);
    }

    if (status == ERR_SUCCESS)
        status = string_catcn(client->header_value, at, length);

    if (status != ERR_SUCCESS) {
        ASSERT(0);
        error_log(DEBUG_CONFIGURATOR, ("HTTP parsing failure\n"), status);
        client->status = HTTP_ERROR;
    }

    return 0;
}

static int on_body(http_parser *self, const char *at, size_t length)
{
    struct client_socket *client = (struct client_socket *)self->data;

    /* If some error occurred before */
    if (client->status == HTTP_ERROR)
        return 0;

    printf("Body: %.*s\n", (int)length, at);
    return 0;
}

static inline void open_server_socket(void)
{
    int32_t status;

    status = cfg_server.net_if->server_socket(cfg_server.net_if,
                                              &cfg_server.server_socket,
                                              cfg_server.ip,
                                              cfg_server.port);
    if (status != ERR_SUCCESS) {
        error_log(DEBUG_CONFIGURATOR, ("Server socket can not be opened.\n"), status);
        cfg_server.server_socket = NULL;
    }

    debug_log(DEBUG_CONFIGURATOR, ("Server socket is opened\n"));
}

static int32_t close_server_socket(void)
{
    cfg_server.net_if->close(cfg_server.server_socket);
    cfg_server.server_socket = NULL;
    debug_log(DEBUG_CONFIGURATOR, ("Server socket is closed\n"));

    return ERR_SUCCESS;
}

static int32_t add_client_socket(struct net_socket *client_socket)
{
    int idx;

    if (cfg_server.connections <= MAX_CONFIGURATOR_CONNECTION) {
        for (idx = 0; idx < MAX_CONFIGURATOR_CONNECTION; idx++) {
            if (cfg_server.client_socket[idx].status == CLOSED) {

                ASSERT(cfg_server.client_socket[idx].socket == NULL);

                cfg_server.client_socket[idx].socket = client_socket;
                http_parser_init(&cfg_server.client_socket[idx].parser, HTTP_REQUEST);
                cfg_server.client_socket[idx].parser.data = &cfg_server.client_socket[idx];
                cfg_server.client_socket[idx].status = OPENED;
                cfg_server.connections++;
                break;
            }
        }
        debug_log(DEBUG_CONFIGURATOR, ("Client socket is added (%d)\n", idx));
        return ERR_SUCCESS;
    } else {
        debug_log(DEBUG_CONFIGURATOR, ("Connection refused. Maximum client number is reached\n"));
        cfg_server.net_if->close(client_socket);
        return ERR_FAILURE;
    }
}

static inline void close_client_socket(size_t idx)
{
    ASSERT(idx < MAX_CONFIGURATOR_CONNECTION);
    ASSERT(cfg_server.client_socket[idx].status != CLOSED);
    ASSERT(cfg_server.client_socket[idx].socket != NULL);

    message_cleanup(&cfg_server.client_socket[idx]);

    cfg_server.net_if->close(cfg_server.client_socket[idx].socket);
    cfg_server.client_socket[idx].socket = NULL;
    cfg_server.client_socket[idx].status = CLOSED;
    cfg_server.connections--;
    debug_log(DEBUG_CONFIGURATOR, ("Client socket is closed (%d)\n", idx));
}

static void send_task(size_t idx)
{
    int32_t status = ERR_SUCCESS;
    struct client_socket *client_socket = &cfg_server.client_socket[idx];
    size_t nbytes_sent;

    ASSERT(client_socket->socket != NULL);
    ASSERT(client_socket->status == OPENED || client_socket->status == SEND_IN_PROGRESS);

    if (client_socket->response != NULL) {

        debug_log(DEBUG_CONFIGURATOR, ("RESPONSE:\n%s\n", (status == ERR_SUCCESS) ?
                                                          string_get_cstring(client_socket->response) :
                                                          "Failure"));

        status = cfg_server.net_if->send(client_socket->socket,
                                         string_get_cstring(client_socket->response) + client_socket->sent,
                                         string_get_size(client_socket->response) - client_socket->sent,
                                         &nbytes_sent);

        switch (status) {

        case ERR_SUCCESS:
            ASSERT(nbytes_sent == strlen(string_get_cstring(client_socket->response) + client_socket->sent));
            string_delete(client_socket->response);
            client_socket->response = NULL;
            client_socket->status = RESPONSE_SENT;
            break;

        case ERR_WOULD_BLOCK:
            client_socket->sent += nbytes_sent;
            ASSERT(nbytes_sent < strlen(string_get_cstring(client_socket->response)));
            client_socket->status = SEND_IN_PROGRESS;
            break;

        default:
            client_socket->status = SOCKET_ERROR;
            break;
        }
    } else if (client_socket->status == SEND_IN_PROGRESS) {
        client_socket->status = RESPONSE_SENT;
    }
}

static void receive_task(size_t idx)
{
    int32_t status = ERR_SUCCESS;
    size_t nbytes_recvd = 0;
    size_t nparsed = 0;
    struct client_socket *client_socket = &cfg_server.client_socket[idx];

    ASSERT(client_socket->socket != NULL);
    status = cfg_server.net_if->recv(client_socket->socket,
                                     cfg_server.input_buff,
                                     CONFIGURATOR_BUFFER_SIZE,
                                     &nbytes_recvd);

    /* If the status is ERR_WOULD_BLOCK there is no data on the socket*/
    if (status == ERR_WOULD_BLOCK)
        return;

    /* Connection error */
    if (status != ERR_SUCCESS) {
        client_socket->status = SOCKET_ERROR;
        return;
    }

    ASSERT(nbytes_recvd <= CONFIGURATOR_BUFFER_SIZE);

    /* socket is closed by the client */
    if (nbytes_recvd == 0) {
        client_socket->status = DISCONNECTED;
        return;
    }

    /* Send the chunk to the parser */
    nparsed = http_parser_execute(&client_socket->parser,
                                  &cfg_server.parser_settings,
                                  cfg_server.input_buff,
                                  nbytes_recvd);

    if ((client_socket->status == HTTP_ERROR) || (nparsed != nbytes_recvd)) {
        error_log(DEBUG_CONFIGURATOR,
                  ("HTTP Parser error: %s (%s)\n",
                  http_errno_description(HTTP_PARSER_ERRNO(&client_socket->parser)),
                  http_errno_name(HTTP_PARSER_ERRNO(&client_socket->parser))),
                  ERR_FAILURE);
        message_cleanup(client_socket);
        return;
    }
}

static void configurator_server_task(void *obj)
{
    struct net_socket *new_client_socket = NULL;
    int32_t status = ERR_SUCCESS;
    size_t i;

    (void)obj;

    /** 1. Check state of the listener socket. Open one it is necessary */
    if (cfg_server.server_socket == NULL)
        open_server_socket();

    /** 2. Accept new connections */
    if (cfg_server.server_socket != NULL) {
        status = cfg_server.net_if->accept(cfg_server.server_socket,
                                           &new_client_socket);
        if (status == ERR_SUCCESS) {
            ASSERT(new_client_socket != NULL);
            debug_log(DEBUG_CONFIGURATOR, ("New HTTP connection request (%d/%d)\n",
                                           cfg_server.connections+1,
                                           MAX_CONFIGURATOR_CONNECTION));
            status = add_client_socket(new_client_socket);
            debug_log(DEBUG_CONFIGURATOR, ("HTTP client %d is connected\n", cfg_server.connections));
        } else if (status != ERR_WOULD_BLOCK) {
            error_log(DEBUG_CONFIGURATOR, ("Connection request failure\n"), status);
            close_server_socket();
        }
    }

    /*
     * 4. Itarate through the active client sockets and
     *    perform the necessary actions
     */
    for (i = 0; i < MAX_CONFIGURATOR_CONNECTION; i++) {

        int32_t app_status = ERR_SUCCESS;
        enum page_type page = PAGE_MAIN;

        /** - Read from the socket if there is no pending write */
        if (cfg_server.client_socket[i].status == OPENED ||
            cfg_server.client_socket[i].status == RECV_IN_PROGRESS)
            receive_task(i);

        /** - In case of WiFi configuration changes, the application callback is called */
        if (cfg_server.cfg_changed == TRUE && cfg_server.clb_set != NULL) {
            app_status = cfg_server.clb_set(&cfg_server.wifi);
            if (app_status == ERR_SUCCESS) {
                page = PAGE_SUCCESS;
            } else {
                page = PAGE_ERROR;
            }
            cfg_server.cfg_changed = FALSE;
        }

        if (cfg_server.client_socket[i].status == MESSAGE_RECVD) {
            status = set_response(&cfg_server.client_socket[i], page);
            if (status != ERR_SUCCESS) {
                error_log(DEBUG_CONFIGURATOR, ("Failed to send response\n"), status);
            }
            cfg_server.client_socket[i].status = SEND_IN_PROGRESS;
        }

        /** - Send response if there is any, and there is no pending read */
        if (cfg_server.client_socket[i].status == OPENED ||
            cfg_server.client_socket[i].status == SEND_IN_PROGRESS)
            send_task(i);

        switch (cfg_server.client_socket[i].status) {

        /** - If the response is sent successfully or the client has closed the socket,
         *    the server side socket will be closed
         */
        case RESPONSE_SENT:
        case DISCONNECTED:
            debug_log(DEBUG_CONFIGURATOR, ("HTTP client %d is disconnected\n", i));
            close_client_socket(i);
            break;

        /** - If some error happened on the socket, it will be closed */
        case SOCKET_ERROR:
            error_log(DEBUG_CONFIGURATOR, ("Configuration Server read error on socket %d\n", i), ERR_FAILURE);
            close_client_socket(i);
            break;

        default:
            break;
        }
    }
}

int32_t start_configurator_server(const char *ip,
                                  uint16_t port,
                                  const char *sn,
                                  int32_t (*on_set)(struct wifi_settings *))
{
    int32_t status;
    const char *dev_sn = sn;

    info_log(DEBUG_CONFIGURATOR, ("Configurator Server started [%s:%d]\n", ip, port));

    if (dev_sn == NULL)
        dev_sn = system_get_sn();

    cfg_server.port = port;
    cfg_server.clb_set = on_set;

    status = sf_mem_alloc((void **)&cfg_server.sn, strlen(dev_sn) + 1);
    if (status != ERR_SUCCESS)
        return status;

    ASSERT(cfg_server.sn && dev_sn);
    strcpy(cfg_server.sn, dev_sn);

    ASSERT(strlen(ip) < sizeof(cfg_server.ip));
    strncpy(cfg_server.ip, ip, sizeof(cfg_server.ip));
    cfg_server.ip[sizeof(cfg_server.ip)-1] = 0;

    cfg_server.net_if = net_dev_tcp_new();
    ASSERT(cfg_server.net_if);

    memset(&cfg_server.parser_settings, 0, sizeof(cfg_server.parser_settings));
    cfg_server.parser_settings.on_message_begin    = on_message_begin;
    cfg_server.parser_settings.on_url              = on_url;
    cfg_server.parser_settings.on_header_field     = on_header_field;
    cfg_server.parser_settings.on_header_value     = on_header_value;
    cfg_server.parser_settings.on_headers_complete = on_headers_complete;
    cfg_server.parser_settings.on_body             = on_body;
    cfg_server.parser_settings.on_message_complete = on_message_complete;

    status = pclb_register(CONFIGURATOR_SERVER_PCLB_PERIOD, configurator_server_task, NULL, NULL);
    if (status != ERR_SUCCESS) {
        error_log(DEBUG_CONFIGURATOR, ("Configuration Server can not be started. (socket error)\n"), status);
        return status;
    }

    return ERR_SUCCESS;
}

int32_t stop_configurator_server(void)
{
    ASSERT(cfg_server.sn);
    sf_mem_free(cfg_server.sn);
    info_log(DEBUG_CONFIGURATOR, ("Configurator Server stopped\n"));

    return ERR_SUCCESS;
}

