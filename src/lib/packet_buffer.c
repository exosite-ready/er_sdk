/**
 * @file packet_buffer.c
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
 * @brief Implements a packet buffer
 *
 **/

#include <string.h>
#include <stdio.h>
#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/sf_malloc.h>
#include <lib/packet_buffer.h>
#include <lib/utils.h>
#include <porting/system_port.h>

struct packet_buffer_entry {
    char *buffer;
    size_t size;
};

struct packet_buffer {
    struct packet_buffer_entry *packets;
    size_t size;
    size_t max_packet_size;
    size_t read_index;
    size_t write_index;
    char *pool;
};

void packet_buffer_push(struct packet_buffer *self, uint8_t *buf, size_t size)
{
    struct packet_buffer_entry *current_packet =
            (struct packet_buffer_entry *) &self->packets[self->write_index];

    system_enter_critical_section();

    if (size == 0) {
        system_leave_critical_section();
        return;
    }

    current_packet->size = UMIN(size, self->max_packet_size);

    /*This assumes that the Packet buffer uses dynamic memory for the buffers*/
    sf_memcpy(current_packet->buffer, buf, current_packet->size);
    self->write_index++;

    if (self->write_index == self->size)
        self->write_index = 0;

    system_leave_critical_section();
}

size_t packet_buffer_pop(struct packet_buffer *self, uint8_t *buf,
        size_t buflen)
{
    size_t to_copy;

    struct packet_buffer_entry *current_packet = (struct packet_buffer_entry *) &self->packets[self->read_index];

    system_enter_critical_section();

    if (current_packet->size == 0) {
        system_leave_critical_section();
        return 0;
    }

    to_copy = UMIN(current_packet->size, buflen);
    memcpy(buf, current_packet->buffer, to_copy);
    current_packet->size = 0;

    self->read_index++;
    if (self->read_index == self->size)
        self->read_index = 0;

    system_leave_critical_section();
    return to_copy;
}

int32_t packet_buffer_new(struct packet_buffer **obj, size_t size, size_t bufsize)
{
    bool status;
    struct packet_buffer *pbuf;
    size_t i;

    status = sf_mem_alloc((void **)&pbuf, sizeof(struct packet_buffer));
    if (status != ERR_SUCCESS)
        return status;

    status = sf_mem_alloc((void **)&pbuf->packets,
            sizeof(struct packet_buffer_entry) * size);
    if (status != ERR_SUCCESS)
        return status;

    status = sf_mem_alloc((void **)&pbuf->pool, size * bufsize);
    if (status != ERR_SUCCESS)
        return status;

    pbuf->max_packet_size = bufsize;
    pbuf->size = size;
    pbuf->read_index = 0;
    pbuf->write_index = 0;

    memset(pbuf->packets, 0, sizeof(struct packet_buffer_entry) * size);

    for (i = 0; i < pbuf->size; i++)
        pbuf->packets[i].buffer = pbuf->pool + i * pbuf->max_packet_size;

    *obj = pbuf;

    return ERR_SUCCESS;
}

void packet_buffer_delete(struct packet_buffer *obj)
{
    sf_free(obj->pool);
    sf_free(obj->packets);
    sf_free(obj);
}
