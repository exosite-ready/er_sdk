/**
 * @file tcp_buffer.c
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
 * @brief A tcp buffer implementation
 *
 * Uses the thread safe circular buffer (ts_cbuffer)
 *
 **/
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/ts_cbuffer.h>
#include <lib/tcp_buffer.h>
#include <lib/sf_malloc.h>
#include <porting/system_port.h>

struct tcp_buffer_class {
    struct cbuffer_class *cbuffer;
    size_t size;
};

struct tcp_buffer_class cbuffer_obj;

int32_t tcp_buffer_new(struct tcp_buffer_class **tcpbuf, size_t size)
{
    struct tcp_buffer_class *cbuf;

    cbuf = sf_malloc(sizeof(*cbuf));
    if (!cbuf)
        return -1;

    memset(cbuf, 0, sizeof(*cbuf));

    ts_cbuffer_new(&cbuf->cbuffer, size);
    if (!cbuf->cbuffer) {
        sf_free(cbuf);
        return -1;
    }

    cbuf->size = size;
    *tcpbuf = cbuf;
    return 0;
}

void tcp_buffer_delete(struct tcp_buffer_class *tcpbuf)
{
    ts_cbuffer_delete(tcpbuf->cbuffer);
    sf_free(tcpbuf);
}

size_t tcp_buffer_pop(struct tcp_buffer_class *tcp_buffer_obj, char *buf, size_t max_size)
{
    return ts_cbuffer_pop(tcp_buffer_obj->cbuffer, buf, max_size);
}

void tcp_buffer_push(struct tcp_buffer_class *tcp_buffer_obj, char *buf,
        size_t size)
{
    size_t i;

    system_enter_critical_section();

    for (i = 0; i < size; i++)
        ts_cbuffer_push(tcp_buffer_obj->cbuffer, *buf++);

    system_leave_critical_section();
}

size_t tcp_buffer_get_free(struct tcp_buffer_class *tcp_buffer_obj)
{
    return ts_cbuffer_get_free(tcp_buffer_obj->cbuffer);
}

size_t tcp_buffer_get_used(struct tcp_buffer_class *tcp_buffer_obj)
{
    return ts_cbuffer_get_used(tcp_buffer_obj->cbuffer);
}
