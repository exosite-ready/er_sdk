/**
 * @file heap_tracker.c
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
 * This file provides a heap tracker implementation
 *
 **/
#include <lib/type.h>
#include <lib/error.h>
#include "heap_space_internal.h"
#include "heap_tracker.h"

#ifdef HEAP_TRACKER_ENABLED
int32_t heap_tracker_init(struct heap_space_class *self)
{
    (void)self;
    return ERR_SUCCESS;
}

int32_t heap_tracker_add_block(struct heap_space_class *self, void *p, size_t size, const char *id)
{
    (void)self;
    (void)p;
    (void)size;
    (void)id;
    return ERR_SUCCESS;
}

int32_t heap_tracker_update_block(struct heap_space_class *self, void *p, size_t new_size, const char *id)
{
    (void)self;
    (void)p;
    (void)new_size;
    (void)id;

    return ERR_SUCCESS;
}

BOOL heap_tracker_is_pointer_valid(struct heap_space_class *self, void *p)
{
    (void)self;
    (void)p;
    return TRUE;
}

BOOL heap_tracker_is_range_valid(struct heap_space_class *self, void *p, size_t size)
{
    (void)self;
    (void)p;
    (void)size;
    return TRUE;
}

void heap_tracker_print_mem_blocks(struct heap_space_class *self)
{
    (void)self;
}

void heap_tracker_print_allocated_memory(struct heap_space_class *self)
{
    (void)self;
}

int32_t heap_tracker_get_usage(struct heap_space_class *self)
{
    (void)self;
    return 0;
}

void heap_tracker_check_memory(struct heap_space_class *self)
{
    (void)self;
}

void *heap_tracker_malloc(struct heap_space_class *self, size_t size)
{
    return self->malloc(self, size);
}

void heap_tracker_free(struct heap_space_class *self, void *p)
{
    self->free(self, p);
}

void *heap_tracker_realloc(struct heap_space_class *self, void *p, size_t new_size)
{
    return self->realloc(self, p, new_size);
}
#endif
