/**
 * @file cbuffer.h
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
 * @brief The circular buffer interface
 *
 **/
#ifndef CBUFFER_H
#define CBUFFER_H

struct cbuffer_class;

/**
 * Creates a circular buffer;
 *
 * @param[out] cbuffer A pointer to the freshly created circular buffer object
 * @param[in]  size    The size of the circular buffer to create
 *
 * @return 0 on success; -1 on error
 **/
int32_t cbuffer_new(struct cbuffer_class **cbuffer, size_t size);

/**
 * Destroys a circular buffer;
 *
 * @param[in] cbuffer A pointer to circular buffer object to be destroyed
 *
 * @returns none
 **/
void cbuffer_delete(struct cbuffer_class *cbuffer);

/**
 * Pops the entire circular buffer content or max_len bytes if it contains more
 * bytes then maxlen;
 *
 * @param[in]  cbuffer A pointer to the circular buffer object
 * @param[out] buf     The buffer to be filled from the circular buffer
 * @param[in]  max_len The size of the buffer
 *
 * @return The number of read bytes
 *
 * */
size_t cbuffer_pop(struct cbuffer_class *cbuffer, char *buf, size_t max_len);

/**
 * Pushes one byte to the circular buffer
 * bytes then maxlen;
 *
 * @param[in]  cbuffer A pointer to the circular buffer object
 * @param[in]  ch      The byte to be pushed
 *
 * @return 0 on success; -1 on error
 **/
void cbuffer_push(struct cbuffer_class *cbuffer, char ch);

/**
 * Gets the size of the free space in the circular buffer
 *
 * @param[in]  cbuffer A pointer to the circular buffer object
 *
 * @return The number of bytes still free in the cbuffer
 **/
size_t cbuffer_get_free(struct cbuffer_class *cbuffer_obj);

/**
 * Gets the size of the used space in the circular buffer
 *
 * @param[in]  cbuffer A pointer to the circular buffer object
 *
 * @return The number of bytes used in the cbuffer
 **/
size_t cbuffer_get_used(struct cbuffer_class *cbuffer_obj);

#endif
