/**
 * @file hs_mem.c
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
 * This file implements the hs_mem interface
 *
 * This implementation is responsible to:
 *     - route 'real' allocator calls
 *       it can be routed to: std malloc, registered malloc; mspace malloc if a memory space is used
 *     - add HEAP instrumentation
 *       The instrumentation is completely modular and is part of an other file.
 *       The instrumentation can be turned on and off it is seemless regarding to this module
 *     - Add Fault injection capabilities
 *     - Add error/debug logs
 *
 * Allocator routing:
 *   - If there was no allocator registered and heap_space is NULL then the calls are routed to
 *     the default allocator (std malloc).
 *   - If there was an allocator registered and heap_space is NULL then the calls are routed to
 *     the registered allocator.
 *   - If heap_space is not NULL then the calls are routed to mspace allocator regardless if there is an
 *     allocator registered or not
 *     If the user wants to use its own allocator he/she should not provide a mem base.
 *     If the user provides a mem base; then the registered allocators will be ignored
 *
 **/

#include <stdio.h>

#include <lib/type.h>
#include <lib/error.h>
#include <lib/debug.h>
#include <lib/dlmalloc.h>
#include <system_utils.h>
#include "heap_space_internal.h"
#include "hs_mem.h"
#include "heap_tracker.h"

/* Switch which can enable emulated out of memory conditions */
static BOOL emulate_out_of_memory = FALSE;

/* Switch to enable logging */
#ifdef HS_MEM_LOG
static BOOL log_enable = FALSE;
#endif

static void *internal_realloc(struct heap_space_class *heap, void *p, size_t size);
static void internal_free(struct heap_space_class *heap, void *p);

/* The internal allocator functions; by default it is the std malloc */
static internal_malloc_func in_malloc = malloc;
static internal_free_func in_free = free;
static internal_realloc_func in_realloc = realloc;

#ifdef HEAP_TRACKER_ENABLED
static malloc_call_func malloc_call = heap_tracker_malloc;
static free_call_func free_call = heap_tracker_free;
static realloc_call_func realloc_call = heap_tracker_realloc;
#else
static malloc_call_func malloc_call = internal_malloc;
static free_call_func free_call = internal_free;
static realloc_call_func realloc_call = internal_realloc;
#endif

void internal_set_allocators(internal_malloc_func imalloc, internal_free_func ifree, internal_realloc_func irealloc)
{
    in_malloc = imalloc;
    in_free = ifree;
    in_realloc = irealloc;
}

void *internal_malloc(struct heap_space_class *heap, size_t size)
{
    if (heap->space == NULL)
        return in_malloc(size);
    else
        return mspace_malloc(heap->space, size);
}

static void *internal_realloc(struct heap_space_class *heap, void *p, size_t size)
{
    if (heap->space == NULL)
        return in_realloc(p, size);
    else
        return mspace_realloc(heap->space, p, size);
}

static void internal_free(struct heap_space_class *heap, void *p)
{
    if (heap->space == NULL)
        in_free(p);
    else
        mspace_free(heap->space, p);
}


int32_t hs_construct(struct heap_space_class *heap, void *base, size_t size, const char *name, int32_t id)
{
    debug_log(DEBUG_MEM, ("Creating mspace %s at %p of size %d\n", name, base, size));
    if (base != NULL) {
        heap->space = create_mspace_with_base(base, size, 0);
        if (heap->space == NULL)
            return ERR_FAILURE;
    } else {
        heap->space = NULL;
    }

    snprintf(heap->name, 8, "%s", name);
    heap->id = id;
    heap->heap_usage = 0;
    heap->peak_heap_usage = 0;
    heap->base = base;
    heap->size = size;
    heap->malloc = internal_malloc;
    heap->free = internal_free;
    heap->realloc = internal_realloc;
    return ERR_SUCCESS;
}

void hs_get_stats(struct heap_space_class *heap)
{
    heap_tracker_print_allocated_memory(heap);
#ifdef PRINT_MEM_BLOCKS
    heap_tracker_print_mem_blocks(heap);
#endif
}

#ifdef HS_MEM_LOG
#define hs_mem_log(message)\
    do {\
        if (log_enable)\
            debug_log(DEBUG_MEM, message);\
    } while (0)
#else
#define hs_mem_log(message)
#endif

int32_t hs_mem_alloc(struct heap_space_class *heap, void **new_ptr, size_t size, const char *id)
{
    char *p;
    char **ret_ptr = (char **)new_ptr;

    ASSERT(new_ptr != NULL);
    ASSERT(size != 0);

    if (emulate_out_of_memory)
        return ERR_NO_MEMORY;

    system_enter_critical_section();

    ASSERT(new_ptr != NULL && size != 0);

    hs_mem_log(("Allocate %d bytes\n", size));
    p = malloc_call(heap, size);
    hs_mem_log(("Allocated %d bytes at %p\n", size));

    heap_tracker_add_block(heap->heap_tracker, p, size, id);

    system_leave_critical_section();

    if (p != NULL) {
        *ret_ptr = p;
        return ERR_SUCCESS;
    } else {
        debug_log(DEBUG_MEM, ("Failed to allocate chunk of size %d from %s Heap\n", size, heap->name));
        hs_mem_log(("Heap usage: %d\n", heap_tracker_get_usage(heap)));
        return ERR_NO_MEMORY;
    }
}

void hs_mem_free(struct heap_space_class *heap, void *ptr)
{
    /* DO not allow freeing NULL pointers */
    ASSERT(ptr != NULL);

    system_enter_critical_section();

    free_call(heap, ptr);

    system_leave_critical_section();
}

int32_t hs_mem_realloc(struct heap_space_class *heap, void **new_ptr, void *old_ptr, size_t size_new, const char *id)
{
    char *p;
    char **ret_ptr = (char **)new_ptr;

    if (emulate_out_of_memory)
        return ERR_NO_MEMORY;

    system_enter_critical_section();

    hs_mem_log(("Realloc internal %p (%d)\n", *old_ptr, size_new));

    ASSERT(old_ptr != NULL);
    ASSERT(size_new != 0);

    p = realloc_call(heap, old_ptr, size_new);
    heap_tracker_update_block(heap->heap_tracker, p, size_new, id);
    system_leave_critical_section();

    if (p != NULL) {
        *ret_ptr = p;
        return ERR_SUCCESS;
    } else {
        return ERR_NO_MEMORY;
    }
}

