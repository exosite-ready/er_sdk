/**
 * @file heap_manager.c
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
 * @brief Initalizes all HEAPs and implements the heap manager allocator interface
 *
 * It creates HEAPs based on static and dynamic configuration using heap_space module
 * After that it uses the heap space module to allocate memory in these memory spaces
 *
 * - It uses the client_config module to determine if there is need for more heap then 1
 *   If there is then it creates these other heaps beside the 'MAIN' heap
 *
 * - If there is need for more heaps then:
 *   - If a memory area was registered then it creates all heaps in this memory area
 *   - If no memory area was registered then it creates HEAPs from the MAIN heap
 *
 **/

#include <lib/type.h>
#include <lib/debug.h>
#include <lib/error.h>
#include <lib/heap_types.h>
#include <lib/mem_check.h>
#include <lib/string_class.h>
#include <client_config.h>
#include <lib/heap_manager.h>
#include "heap_space_internal.h"
#include "hs_mem.h"
#include "heap_tracker.h"

static struct heap_space_class heap_array[MAX_NUMBER_OF_HEAPS];

static struct heap_space_class *hm_find_heap(int32_t id);

void *ersdk_sbrk(ptrdiff_t incr);
void *ersdk_sbrk(ptrdiff_t incr)
{
    int32_t i;
    (void)incr;

    debug_log(DEBUG_MEM, ("Call to sbrk %d\n", incr));
    for (i = 0; i < MAX_NUMBER_OF_HEAPS; i++)
        if (heap_array[i].id != 0)
            hs_get_stats(&heap_array[i]);
    return NULL;
}

void print_memory_stats(void)
{
#ifdef DEBUG_MEMORY_MANAGER
    int32_t i;

    for (i = 0; heap_array[i].id != 0 && i < MAX_NUMBER_OF_HEAPS; i++)
        hs_get_stats(&heap_array[i]);
#endif
}

int32_t hm_mem_alloc(int32_t heap_id, void **ptr, size_t size, const char *chunk_id)
{
    static struct heap_space_class *h;

    h = hm_find_heap(heap_id);
    ASSERT(h);
    if (!h)
        return ERR_FAILURE;

    return hs_mem_alloc(h, ptr, size, chunk_id);
}

int32_t hm_mem_realloc(int32_t heap_id, void **new_ptr, void *old_ptr, size_t size_new, const char *chunk_id)
{
    static struct heap_space_class *h;

    h = hm_find_heap(heap_id);
    ASSERT(h);
    if (!h)
        return ERR_FAILURE;

    return hs_mem_realloc(h, new_ptr, old_ptr, size_new, chunk_id);
}

void hm_mem_free(int32_t heap_id, void *p)
{
    static struct heap_space_class *h;

    h = hm_find_heap(heap_id);
    ASSERT(h);
    if (!h)
        return;

    hs_mem_free(h, p);
}

void *hm_malloc(int32_t heap_id, size_t size, const char *id)
{
    void *pbBlock;

    if (hm_mem_alloc(heap_id, &pbBlock, size, id) == ERR_SUCCESS)
        return pbBlock;
    else
        return NULL;
}

void *hm_realloc(int32_t heap_id, void *p, size_t size, const char *id)
{
    int32_t error;
    uint8_t *pb = p;

    if (pb)
        error = hm_mem_realloc(heap_id, &p, p, size, id);
    else
        error = hm_mem_alloc(heap_id, &p, size, id);

    if (error == ERR_SUCCESS)
        return p;
    else
        return NULL;
}

void hm_free(int32_t heap_id, void *p)
{
    hm_mem_free(heap_id, p);
}

void hm_memfill(int32_t heap_id, void *p, uint8_t b, size_t size)
{
    static struct heap_space_class *h;

    h = hm_find_heap(heap_id);
    ASSERT(h);
    if (!h)
        return;

    ASSERT(heap_tracker_is_range_valid(h, p, size));
    memset(p, b, size);
}

void hm_memcpy(int32_t heap_id, void *dst, const void *src, size_t size)
{
    static struct heap_space_class *h;

    h = hm_find_heap(heap_id);
    ASSERT(h);
    if (!h)
        return;

    ASSERT(heap_tracker_is_range_valid(h, dst, size));
    memcpy(dst, src, size);
}

static struct heap_space_class *hm_find_heap(int32_t heap_id)
{
    int32_t i;

    for (i = 0; i < MAX_NUMBER_OF_HEAPS; i++)
        if (heap_array[i].id == heap_id)
            return &heap_array[i];

    /*
     * A heap corresponding to the heap_id was not found
     * That means that some module is trying to use a heap that was not configured.
     * Please check heap_config.h for the heap ID printed in the error message
     **/
    error_log(DEBUG_MEM, ("Heap with id %d is not configured\n", heap_id), ERR_FAILURE);
    ASSERT(0);

    return NULL;
}

int32_t hm_init(void *base, size_t size)
{
    int32_t error;
    struct heap_config **conf;
    char *heap_base;
    struct heap_space_class *heap;
    int32_t i;
    size_t config_size = 0;
    struct heap_config **global_heap_config;

    global_heap_config = client_config_get_heap_config();
    for (conf = global_heap_config; *conf != NULL; conf++)
        config_size++;

    if (config_size + 1 > MAX_NUMBER_OF_HEAPS) {
        error_log(DEBUG_MEM,
                 ("Configured to use %d heaps but maximum number of heaps is limited to %d\n",
                   config_size, MAX_NUMBER_OF_HEAPS), -1);
        ASSERT(0);
        return ERR_FAILURE;
    }

    if (base) {
        size_t total_size = 0;
        size_t main_heap_size;

        heap = heap_array;
        heap_base = base;
        for (conf = global_heap_config; *conf != NULL; conf++) {
            error = hs_construct(heap++, heap_base, conf[0]->size, conf[0]->name, conf[0]->id);
            heap_base += conf[0]->size;
            total_size += conf[0]->size;
        }
        main_heap_size = size - total_size;
        ASSERT(main_heap_size > 0);
        error = hs_construct(heap, heap_base, main_heap_size, "Main", MAIN_HEAP);
        if (error) {
            error_log(DEBUG_MEM, ("Error construction heap spaces\n"), error);
            return error;
        }


    } else {
        size_t total_size = 0;

        heap = heap_array;

        for (conf = global_heap_config; *conf != NULL; conf++)
            total_size += conf[0]->size;

        hs_construct(heap++, NULL, 0, "Normal", MAIN_HEAP);

        heap_base = internal_malloc(&heap_array[0], total_size);
        if (!heap_base) {
            error_log(DEBUG_MEM, ("Could not allocate memory (%d bytes) for HEAPs\n", total_size), -1);
            ASSERT(0);
            return ERR_FAILURE;
        }

        for (conf = global_heap_config; *conf != NULL; conf++) {
            error = hs_construct(heap++, heap_base, conf[0]->size, conf[0]->name, conf[0]->id);
            heap_base += conf[0]->size;
        }

    }

    for (i = 0; i < MAX_NUMBER_OF_HEAPS; i++)
        if (heap_array[i].id != 0)
            mem_check_register_heap(&heap_array[i]);

    return ERR_SUCCESS;
}


