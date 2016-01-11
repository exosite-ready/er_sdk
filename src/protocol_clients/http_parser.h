/**
 * @file http_parser.h
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
 * @brief An interface to parse http headers
 *
 **/
#ifndef INC_HTTP_PARSER_H_
#define INC_HTTP_PARSER_H_

/**
 * @brief Parse a buffer of characters looking for a HTTP header
 *        Extract result code, content length and payload position if
 *        a HTTP header is found
 *
 * @param[in]  buffer           The buffer to be parsed
 * @param[in]  size             The size of the buffer
 * @param[out] result_code      The extracted response code  (Valid if a HTTP header was found)
 * @param[out] content_length   The extracted content length (Valid if a HTTP header was found)
 * @param[out] payload_index    The position of the payload  (Valid if a HTTP header was found)
 *
 * @return TRUE if a header was found; FALSE if it is not found
 **/
bool http_parse_header(char *buffer, size_t size,
                       int32_t *result_code,
                       size_t *content_length,
                       size_t *payload_index);

#endif /* INC_HTTP_PARSER_H_ */
