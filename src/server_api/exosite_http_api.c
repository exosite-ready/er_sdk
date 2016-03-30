/**
 * @file exosite_http_api.c
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
 * This file implements the Exosite Server specific HTTP API
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/sf_malloc.h>
#include <lib/string_class.h>
#include <lib/debug.h>
#include <lib/fault_injection.h>
#include <lib/pmatch.h>
#include <lib/error.h>
#include <porting/net_port.h>
#include <client_config.h>
#include <exosite_api_internal.h>
#include <server_api.h>

static const char *data_procedures_url = "/onep:v1/stack/alias";
static const char *activate_procedure_url = "/provision/activate";
static const char crlf_str[] = "\r\n";

enum provisioning_procedure {
    PP_ACTIVATE,
};

enum http_type {
    HTTP_POST, HTTP_GET
};

struct http_context {
    struct net_socket *socket;
    struct string_class *request;
};

static int32_t http_create_request(void *req, const struct exo_request_args *args);

#ifdef TEST_ERSDK
static enum fault_type constructor_fault = FT_NO_ERROR;
static int32_t fault_number;

static int32_t http_request_new_simulate_failure(void)
{
    if (fault_number++ > 5 && constructor_fault == FT_ERROR_AFTER_INIT)
        return ERR_FAILURE;

    return ERR_SUCCESS;
}
#endif

static int32_t http_request_new(void **pdu, void *args)
{
    int32_t error;
    struct exo_request_args *request_args = args;
    struct string_class *request;

#ifdef TEST_ERSDK
    error = http_request_new_simulate_failure();
    if (error != ERR_SUCCESS)
        return error;
#endif

    error = string_new(&request, "");
    if (error != ERR_SUCCESS) {
        error_log(DEBUG_MEM, ("Failed to create HTTP request\n"), error);
        return error;
    }

    error = http_create_request(request, request_args);
    if (error != ERR_SUCCESS)
        return error;

    *pdu = request;
    return ERR_SUCCESS;
}

static int32_t http_request_delete(void *pdu)
{
    struct string_class *str = pdu;

    string_delete(str);

    return ERR_SUCCESS;
}

static inline int32_t append_http_header_common(struct string_class *req)
{
    int32_t status;

    status = string_catc(req, " HTTP/1.1\r\n");
    if (status != 0)
        return status;

    status = string_catc(req, "Host: m2.exosite.com\r\n");
    if (status != 0)
        return status;

    return status;
}

static inline int32_t append_http_provisioning_header(struct string_class *req, enum provisioning_procedure procedure)
{
    int32_t status;

    status = string_catc(req, "POST ");
    if (status != 0)
        return status;

    switch (procedure) {
    case PP_ACTIVATE:
        status = string_catc(req, activate_procedure_url);
        if (status != 0)
            return status;
        break;
    default:
        ASSERT(0);
        return ERR_FAILURE;
    }

    status = append_http_header_common(req);
    return status;
}


static inline int32_t append_http_data_proc_header(struct string_class *req,
        enum http_type type, const char *alias)
{
    int32_t status = ERR_SUCCESS;

    switch (type) {
    case HTTP_POST:
        status = string_catc(req, "POST ");
        break;
    case HTTP_GET:
        status = string_catc(req, "GET ");
        break;
    default:
        break;
    }
    if (status != 0)
        return status;

    status = string_catc(req, data_procedures_url);
    if (status != 0)
        return status;

    switch (type) {
    case HTTP_POST:
        break;
    case HTTP_GET:
        status = string_catc(req, "?");
        break;
    default:
        break;
    }

    status = string_catc(req, alias);
    if (status != 0)
        return status;

    status = append_http_header_common(req);
    return status;
}

static inline int32_t append_http_utility_header(struct string_class *req,
                                                   enum http_type type,
                                                   const char *url)
{
    int32_t error;

    switch (type) {
    case HTTP_POST:
        error = string_catc(req, "POST ");
        break;
    case HTTP_GET:
        error = string_catc(req, "GET ");
        break;
    default:
        break;
    }

    error = string_catc(req, url);
    if (error != ERR_SUCCESS)
        return error;

    error = append_http_header_common(req);
    return error;
}

static inline int32_t append_cik_str(struct string_class *req, const char *cik)
{
    int32_t status = ERR_SUCCESS;

    status = string_catc(req, "X-Exosite-CIK: ");
    if (status != 0)
        return status;

    status = string_catc(req, cik);
    if (status != 0)
        return status;

    status = string_catc(req, crlf_str);
    if (status != 0)
        return status;

    return status;
}

static inline int32_t append_accept_str(struct string_class *req)
{
    return (string_catc(req,
            "Accept: application/x-www-form-urlencoded; charset=utf-8\r\n"));
}

static inline int32_t append_content_str(struct string_class *req,
        struct string_class *content)
{
#define LENGTH_STRING_MAX_SIZE 16

    int32_t status = ERR_SUCCESS;

    char length_str[LENGTH_STRING_MAX_SIZE];

    status =  string_catc(req,
                    "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n");
    if (status != 0)
        return status;

    status = string_catc(req, "Content-Length: ");
    if (status != 0)
        return status;

    snprintf(length_str, LENGTH_STRING_MAX_SIZE, "%lu",
            (unsigned long)string_get_size(content));
    status = string_catc(req, length_str);
    if (status != 0)
        return status;

    status = string_catc(req, crlf_str);
    if (status != 0)
        return status;

    status = string_catc(req, crlf_str); /*(blank line)*/
    if (status != 0)
        return status;

    status = string_cat(req, content);
    if (status != 0)
        return status;

    return status;
}

static int32_t http_create_write_request(struct string_class *req, const char *cik,
        const char *alias, const char *value)
{
    int32_t status = ERR_SUCCESS;
    struct string_class *content;

    status = append_http_data_proc_header(req, HTTP_POST, "");
    if (status != 0)
        return status;

    status = append_cik_str(req, cik);
    if (status != 0)
        return status;

    status = string_new(&content, "");
    if (status != 0)
        return status;

    status = string_catc(content, alias);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, "=");
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, value);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_content_str(req, content);
    if (status != ERR_SUCCESS)
        goto cleanup;

cleanup: string_delete(content);
    return status;
}

static int32_t http_create_read_request(struct string_class *req, const char *cik,
        const char *alias)
{
    int32_t status = ERR_SUCCESS;

    status = append_http_data_proc_header(req, HTTP_GET, alias);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_cik_str(req, cik);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_accept_str(req);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(req, crlf_str); /*(blank line)*/
    if (status != ERR_SUCCESS)
        goto cleanup;

cleanup:
    return status;
}

static int32_t http_create_hybrid_wr_request(struct string_class *req,
        const char *cik, const char *alias, const char *value)
{
    /*TODO*/
    (void)req;
    (void)cik;
    (void)alias;
    (void)value;

    return ERR_NOT_IMPLEMENTED;
}

static int32_t http_create_long_polling_request(struct string_class *req,
        const char *cik, const char *alias, const char *timeout,
        const char *timestamp)
{
    int32_t status = ERR_SUCCESS;

    /* Add timestamp usage*/
    (void)timestamp;

    status = append_http_data_proc_header(req, HTTP_GET, alias);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_cik_str(req, cik);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_accept_str(req);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(req, "Request-Timeout: ");
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(req, timeout);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(req, crlf_str);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(req, crlf_str); /*(blank line)*/
    if (status != ERR_SUCCESS)
        goto cleanup;

cleanup:
    return status;
}

static int32_t http_create_provisioning_request(struct string_class *req, enum provisioning_procedure procedure,
        const char *vendor, const char *model, const char *sn)
{
    int32_t status = ERR_SUCCESS;
    struct string_class *content;

    status = append_http_provisioning_header(req, procedure);
    if (status != 0)
        return status;

    status = string_new(&content, "vendor=");
    if (status != 0)
        return status;

    status = string_catc(content, vendor);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, "&model=");
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, model);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, "&sn=");
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = string_catc(content, sn);
    if (status != ERR_SUCCESS)
        goto cleanup;

    status = append_content_str(req, content);
    if (status != ERR_SUCCESS)
        goto cleanup;

cleanup: string_delete(content);
    return status;
}

static int32_t http_create_list_av_content_request(struct string_class *req,
        const char *cik, const char *vendor, const char *model)
{
    /*TODO*/
    (void)req;
    (void)cik;
    (void)vendor;
    (void)model;

    return ERR_NOT_IMPLEMENTED;
}

static int32_t http_create_download_content_request(struct string_class *req,
        const char *cik, const char *vendor, const char *model, const char *id)
{
    /*TODO*/
    (void)req;
    (void)cik;
    (void)vendor;
    (void)model;
    (void)id;

    return ERR_NOT_IMPLEMENTED;
}

static int32_t http_create_timestamp_request(struct string_class *req)
{
    int32_t error;

    error = append_http_utility_header(req, HTTP_GET, "/timestamp");
    if (error != ERR_SUCCESS)
        return error;
    error = append_accept_str(req);
    if (error != ERR_SUCCESS)
        return error;

    error = string_catc(req, crlf_str); /*(blank line)*/

    return error;
}

static int32_t http_create_ip_request(struct string_class *req)
{
    /*TODO*/
    (void)req;

    return ERR_NOT_IMPLEMENTED;
}

static int32_t http_create_request(void *req, const struct exo_request_args *args)
{
    int32_t status;
    struct string_class *request = (struct string_class *) req;

    switch (args->method) {

    /* Data Procedures */
    case EXO_DP_WRITE:
        status = http_create_write_request(request, args->cik, args->alias,
                args->value);
        break;
    case EXO_DP_READ:
        status = http_create_read_request(request, args->cik, args->alias);
        break;
    case EXO_DP_READ_WRITE:
        status = http_create_hybrid_wr_request(request, args->cik, args->alias,
                args->value);
        break;
    case EXO_DP_SUBSCRIBE:
        status = http_create_long_polling_request(request, args->cik,
                args->alias, args->timeout, args->timestamp);
        break;

        /* Provisioning Procedures */
    case EXO_PP_ACTIVATE:
        status = http_create_provisioning_request(request, PP_ACTIVATE, args->vendor,
                args->model, args->sn);
        break;
    case EXO_PP_LIST_CONTENT:
        status = http_create_list_av_content_request(request, args->cik,
                args->vendor, args->model);
        break;
    case EXO_PP_DOWNLOAD_CONTENT:
        status = http_create_download_content_request(request, args->cik,
                args->vendor, args->model, args->id);
        break;

        /* Utility Procedures */
    case EXO_UP_TIMESTAMP:
        status = http_create_timestamp_request(request);
        break;
    case EXO_UP_IP:
        status = http_create_ip_request(request);
        break;

    default:
        return ERR_NOT_IMPLEMENTED;
    }

#ifdef DEBUG_HTTP_CLIENT
    debug_log(DEBUG_HTTP, ("========\n%s\n=========\n", string_get_cstring(request)));
#endif

    return status;
}

/*
 * It seems that the response codes for COAP does not follow the coap specification
 * until this is clarified here we try to handle both COAP and HTTP
 **/
static int32_t response_code_to_system_error(int32_t response_code, enum exosite_method method)
{
    (void)method;

    if (response_code < 0)
        return response_code;

    switch (response_code) {
    case 204:
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

/* TODO multiple values should be handled */
static int32_t http_parse_payload(struct exo_request_args *request_args,
                                  char *payload,
                                  size_t size,
                                  size_t max_size,
                                  int32_t response_code)
{
    struct capture captures[10];
    size_t num_captures;
    int32_t status = ERR_INVALID_FORMAT;
    bool found;
    const char *pattern;

    status = response_code_to_system_error(response_code, request_args->method);
    if (status != ERR_SUCCESS) {
        if (status == ERR_NO_CONTENT)
            return ERR_SUCCESS;
        else
            return status;
    }

    switch (request_args->method) {
    case EXO_PP_ACTIVATE:
    case EXO_UP_TIMESTAMP:
        pattern = "(%w*)";
        found = str_find(payload, size, pattern, strlen(pattern), 0, 0, &num_captures, captures, 10);
        if (found) {
            if (captures[0].len + 1 < max_size)
                payload[captures[0].len] = '\0';
        }
        break;
    case EXO_DP_WRITE:
    case EXO_DP_READ:
    case EXO_DP_READ_WRITE:
    case EXO_DP_SUBSCRIBE:
    case EXO_PP_LIST_CONTENT:
    case EXO_PP_DOWNLOAD_CONTENT:
    case EXO_UP_IP:
    default:
        pattern = "(%w-)=([%-%w]*)";
        found = str_find(payload, size, pattern, strlen(pattern), 0, 0, &num_captures, captures, 10);
        if (found) {
            memmove(payload, captures[1].init, captures[1].len);
            if (captures[1].len + 1 < max_size)
                payload[captures[1].len] = '\0';
        }

        break;
    }

    if (!found)
        return ERR_INVALID_FORMAT;

    return status;
}

static struct server_api_request_ops server_api = {
        http_request_new,
        http_request_delete,
        http_parse_payload
};

void exosite_http_api_request_new(struct server_api_request_ops **obj)
{
    *obj = &server_api;
}
