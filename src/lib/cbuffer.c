/**
 * @file cbuffer.c
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
 * A circular buffer implementation
 **/
#include <stdlib.h>
#include <string.h>
#include <lib/type.h>
#include <lib/cbuffer.h>
#include <lib/sf_malloc.h>

struct cbuffer_class {
    char *buffer;
    size_t read_pointer;
    size_t write_pointer;
    size_t size;
};

int32_t cbuffer_new(struct cbuffer_class **cbuffer, size_t size)
{
    struct cbuffer_class *cb;

    cb = sf_malloc(sizeof(*cb));
    if (!cb)
        return -1;

    memset(cb, 0, sizeof(*cb));

    cb->buffer = sf_malloc(size);
    if (!cb->buffer) {
        sf_free(cb);
        return -1;
    }

    cb->size = size;
    *cbuffer = cb;
    return 0;
}

void cbuffer_delete(struct cbuffer_class *cbuffer)
{
    sf_free(cbuffer->buffer);
    sf_free(cbuffer);
}

size_t cbuffer_pop(struct cbuffer_class *cbuffer_obj, char *buf, size_t max_len)
{
    size_t read = 0;

    while (read < max_len  && cbuffer_obj->read_pointer != cbuffer_obj->write_pointer) {
#ifdef CBUFFER_DEBUG
        printf("rp: %d, wp: %d\r\n", cbuffer_obj->read_pointer, cbuffer_obj->write_pointer);
#endif

        *buf++ = cbuffer_obj->buffer[cbuffer_obj->read_pointer++];
        if (cbuffer_obj->read_pointer == cbuffer_obj->size)
            cbuffer_obj->read_pointer = 0;
        read++;
    }

    return read;
}

void cbuffer_push(struct cbuffer_class *cbuffer_obj, char ch)
{
#ifdef CBUFFER_DEBUG
    printf("ch: %c, wp: %d\r\n", ch, cbuffer_obj.write_pointer);
#endif
    cbuffer_obj->buffer[cbuffer_obj->write_pointer++] = ch;
    if (cbuffer_obj->write_pointer == cbuffer_obj->size)
        cbuffer_obj->write_pointer = 0;
}

size_t cbuffer_get_free(struct cbuffer_class *cbuffer_obj)
{
#ifdef CBUFFER_DEBUG
    printf("Write %d read %d\n", cbuffer_obj->write_pointer, cbuffer_obj->read_pointer);
#endif
    return cbuffer_obj->size - cbuffer_get_used(cbuffer_obj);
}

size_t cbuffer_get_used(struct cbuffer_class *cbuffer_obj)
{
    if (cbuffer_obj->write_pointer >= cbuffer_obj->read_pointer)
        return cbuffer_obj->write_pointer - cbuffer_obj->read_pointer;
    else
        return cbuffer_obj->size - (cbuffer_obj->read_pointer - cbuffer_obj->write_pointer);
}
