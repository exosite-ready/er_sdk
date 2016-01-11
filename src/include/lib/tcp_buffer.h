/**
 * @file tcp_buffer.h
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
 * @brief The tcp buffer interface
 *
 * Currently used by the UIP stack wrappers
 * UIP does not provide a TCP buffer; there is only one packet buffer in uip
 **/

#ifndef INC_TCP_BUFFER_H_
#define INC_TCP_BUFFER_H_

#include <stdint.h>

struct tcp_buffer_class;
/**
 * @brief Creates a new TCP buffer with the specified size
 *
 * @param[out] *tcpbuf   The newly created object
 * @param[in]  size      The size of the buffer in bytes
 **/
int32_t tcp_buffer_new(struct tcp_buffer_class **tcpbuf, size_t size);

/**
 * @brief Deletes the TCP buffer object
 *
 * @param[in] tcpbuf   The object to be deleted
 **/
void tcp_buffer_delete(struct tcp_buffer_class *tcpbuf);

/**
 * @brief Push bytes to the buffer
 *
 * @param[in] tcp_buffer_obj    The object itself
 * @param[in] buf               The buffer to be pushed
 * @param[in] size              The size of the buffer
 *
 **/
void tcp_buffer_push(struct tcp_buffer_class *tcp_buffer_obj, char *buf, size_t size);

/**
 * @brief Pops bytes from the TCP buffer
 *
 * @param[in]  tcp_buffer_obj    The object itself
 * @param[out] buf               The buffer where the bytes are copied from the tcp buffer
 * @param[in]  max_size          The size of the output buffer
 *
 * @return The number of bytes popped
 **/
size_t tcp_buffer_pop(struct tcp_buffer_class *tcp_buffer_obj, char *buf, size_t max_size);

/**
 * @brief Get the number of used bytes from the tcp buffer
 *
 * @param[in] tcpbuf   The object itself
 *
 * @return The number of used bytes
 **/
size_t tcp_buffer_get_used(struct tcp_buffer_class *tcp_buffer_obj);

/**
 * @brief Get the number of bytes in the tcp buffer that are still unused
 *
 * @param[in] tcpbuf   The object itself
 *
 * @return The number of unused bytes
 **/
size_t tcp_buffer_get_free(struct tcp_buffer_class *tcp_buffer_obj);
#endif /* INC_TCP_BUFFER_H_ */
