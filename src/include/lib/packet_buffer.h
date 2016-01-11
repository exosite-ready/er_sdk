/**
 * @file packet_buffer.h
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
 * @brief The packet buffer interface
 *
 * This is used by raw ethernet device drivers
 **/

#ifndef PACKET_BUFFER_H_
#define PACKET_BUFFER_H_

struct packet_buffer;

/**
 * Creates a new packet buffer object
 *
 * @param[out] obj  The freshly created object
 *
 * @return ERR_SUCCESS on success or an error code otherwise
 *
 **/
int32_t packet_buffer_new(struct packet_buffer **obj, size_t size, size_t bufsize);

/**
 * Gets a packet from the packet buffer
 *
 * @param[in] self  The packet buffer object
 * @param[out] buf  The output buffer where the data is copied
 * @param[out] buflen The length of the buffer
 *
 * @return 0 if the packet buffer was empty, or the size of the data copied into buf
 **/
size_t packet_buffer_pop(struct packet_buffer *self, uint8_t *buf,
        size_t buflen);

/**
 * Puts a packet to the packet buffer
 *
 * @param[in] self  The packet buffer object
 * @param[in] buf  The data to be put into the packet buffer
 * @param[in] buflen The length of the input buffer
 *
 * @return Always succeeds
 **/
void packet_buffer_push(struct packet_buffer *self, uint8_t *buf, size_t size);

void packet_buffer_delete(struct packet_buffer *obj);
#endif /* PACKET_BUFFER_H_ */
