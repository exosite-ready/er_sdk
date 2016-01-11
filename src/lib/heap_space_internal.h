/**
 * @file heap_space_internal.h
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
 * This file provides the internal allocator interface for heap_space and heap_space memcheck modules;
 * it uses the standard allocator prototypes
 *
 **/

#ifndef ER_SDK_SRC_LIB_HEAP_SPACE_INTERNAL_H_
#define ER_SDK_SRC_LIB_HEAP_SPACE_INTERNAL_H_

typedef void *(*internal_malloc_func)(size_t);
typedef void  (*internal_free_func)(void *);
typedef void *(*internal_realloc_func)(void *, size_t);

#ifdef DEBUG_EASDK
#define HEAP_TRACKER_ENABLED
#endif

struct heap_space_class;
typedef void *(*malloc_call_func)(struct heap_space_class *, size_t);
typedef void  (*free_call_func)(struct heap_space_class *, void *);
typedef void *(*realloc_call_func)(struct heap_space_class *, void *, size_t);

struct heap_space_class {
    /*We use dlmalloc's mspace, which is void* as well; and like this we don't create a dependency with it*/
    void *space;
    size_t heap_usage;
    size_t peak_heap_usage;
    char   name[8];         /* A short name*/
    int32_t id;             /* The id of the heap*/
    void *base;             /* The base pointer of this heap; optional*/
    size_t size;            /* The size of this heap; optional */
    void *heap_tracker;     /* Pointer to the tracking information; only if HEAP_TRACKER_ENABLED is set*/
    malloc_call_func malloc;
    free_call_func free;
    realloc_call_func realloc;
};

/**
 *  @brief Registers allocators to the internal_allocator module
 *
 *  @param[in] imalloc     The malloc function to be registered
 *  @param[in] ifree       The free function to be registered
 *  @param[in] irealloc    The realloc function to be registered
 **/
void internal_set_allocators(internal_malloc_func imalloc, internal_free_func ifree, internal_realloc_func irealloc);

void *internal_malloc(struct heap_space_class *heap, size_t size);
#endif /* ER_SDK_SRC_LIB_HEAP_SPACE_INTERNAL_H_ */
