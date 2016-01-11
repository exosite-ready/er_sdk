/**
 * @file ts_buffer.c
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
 * This module implements a thread safe circular buffer
 **/
#include <lib/type.h>
#include <lib/cbuffer.h>
#include <lib/ts_cbuffer.h>
#include <porting/system_port.h>

int32_t ts_cbuffer_new(struct cbuffer_class **cbuffer, size_t size)
{
    return cbuffer_new(cbuffer, size);
}

void ts_cbuffer_delete(struct cbuffer_class *cbuffer)
{
    cbuffer_delete(cbuffer);
}

size_t ts_cbuffer_pop(struct cbuffer_class *cbuffer_obj, char *buf, size_t max_len)
{
    size_t read = 0;

    system_enter_critical_section();
    read = cbuffer_pop(cbuffer_obj, buf, max_len);
    system_leave_critical_section();

    return read;
}

void ts_cbuffer_push(struct cbuffer_class *cbuffer_obj, char ch)
{
    system_enter_critical_section();
    cbuffer_push(cbuffer_obj, ch);
    system_leave_critical_section();
}

size_t ts_cbuffer_get_free(struct cbuffer_class *cbuffer_obj)
{
    size_t free_ = 0;

    system_enter_critical_section();
    free_ = cbuffer_get_free(cbuffer_obj);
    system_leave_critical_section();

    return free_;
}

size_t ts_cbuffer_get_used(struct cbuffer_class *cbuffer_obj)
{
    size_t used = 0;

    system_enter_critical_section();
    used = cbuffer_get_used(cbuffer_obj);
    system_leave_critical_section();

    return used;
}

