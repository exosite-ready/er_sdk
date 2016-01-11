/**
 * @file http_parser.c
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
 * @brief Implements http header parsing
 *
 **/
#include <string.h>
#include <stdlib.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/string_class.h>
#include <lib/pmatch.h>
#include <assert.h>
#include "http_parser.h"

bool http_parse_header(char *buffer, size_t buffer_size,
                       int32_t *result_code,
                       size_t *content_length,
                       size_t *payload_index)
{
    int32_t status;
    struct capture captures[10];
    size_t num_captures;
    bool found;
    struct string_class *result_str;
    struct string_class *content_length_str;
    char *endptr;
    char *cstr;
    const char *packet_end_pattern = "(.*)\r\n\r\n";
    const char *http_header_pattern = "HTTP/1%.1 (%d+)";
    const char *packet_length_pattern = "Content%-Length: (%d+)";

    found = str_find(buffer,
                     buffer_size,
                     packet_end_pattern,
                     strlen(packet_end_pattern),
                     0,
                     0,
                     &num_captures,
                     captures,
                     10);

    /*printf("%p, %d / %d", captures[0].init, captures[0].len + 4, string_get_size(str));*/
    *payload_index = captures[0].len + 4;
    if (!found)
        return found;

    found = str_find(buffer,
                     buffer_size,
                     http_header_pattern,
                     strlen(http_header_pattern),
                     0,
                     0,
                     &num_captures,
                     captures,
                     10);

    if (!found)
        return found;

    status = string_new_const(&result_str, captures[0].init, captures[0].len);
    if (status != ERR_SUCCESS)
        return FALSE;

    cstr = string_get_cstring(result_str);
    *result_code = (int32_t)strtol(cstr, &endptr, 0);
    if (endptr == cstr || *endptr != '\0') {
        string_delete(result_str);
        return FALSE;
    }

    /*debug_log(DEBUG_HTTP, ("HTTP Result code: %d\n\n", *result_code));*/
    found = str_find(buffer,
                     buffer_size,
                     packet_length_pattern,
                     strlen(packet_length_pattern),
                     0,
                     0,
                     &num_captures,
                     captures,
                     10);

    if (!found) {
        string_delete(result_str);
        return found;
    }

    status = string_new_const(&content_length_str, captures[0].init, captures[0].len);
    if (status != ERR_SUCCESS) {
        string_delete(result_str);
        return FALSE;
    }

    cstr = string_get_cstring(content_length_str);
    *content_length = (size_t)(strtol(cstr, &endptr, 0));  /* TODO Casting to size_t from long int is not safe! */
    if (endptr == cstr || *endptr != '\0')
        found = FALSE;

    string_delete(result_str);
    string_delete(content_length_str);
    return found;
}
