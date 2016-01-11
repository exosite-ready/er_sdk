/**
 * @file sf_malloc.h
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
 * @brief Interface to an instrumented malloc implementation to allow to catch bugs early
 *
 * Features:
 *   - Support for several heaps
 *     By default the MAIN heap is used, but this interface can allocate memory
 *     from various heaps depending on the HEAP_MODULE_ID; if in a module a
 *     specific heap is needed then you should #define HEAP_MODULE_ID to the
 *     heap type before sf_malloc.h is included
 *   - sf_malloc, realloc, free calls are thread safe
 *   - In debug mode instrumentation is added
 *     Memory checks occur when malloc, free, etc is used or can be forced by
 *     calling check_memory
 *   - In debug mode if USE_MEM_ID is defined it will add a memory ID which
 *     makes it easier to track down leaking or faulty memory blocks
 *
 *
 * The public API are *preprocessor macros* so that:
 *     - HEAP selection can be switched while the malloc prototype can still remain the same
 *     - memory IDs can be used seamlessly
 **/
#ifndef SF_MALLOC_H
#define SF_MALLOC_H

#include <lib/type.h>
#include <lib/heap_types.h>
#include <lib/heap_manager.h>
#include <sdk_config.h>
struct heap_space_class;

/*Specify memory IDs which is the location of the sf_ call in the source file*/
#ifdef USE_MEM_ID
#define LOCATION_IN_FILE __FILE__ " at " MAKE_STRING(__LINE__)
#else
#define LOCATION_IN_FILE NULL
#endif

#ifndef HEAP_MODULE_ID
#define HEAP_MODULE_ID MAIN_HEAP
#endif

/**
 * @brief New style malloc
 *
 * It internally uses hm_mem_alloc
 *
 * The heap it uses to allocate from depends on the predefined HEAP_MODULE_ID, by
 * default it uses the 'MAIN' heap
 * If USE_MEM_ID is turned on then it will add an id for every allocated chunk which
 * can be used to track memory errors
 *
 * @param[out] *p  The pointer to the allocated buffer
 * @param[in]  size  The size of bytes to be allocated
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
#define sf_mem_alloc(p, size) hm_mem_alloc(HEAP_MODULE_ID, p, size, LOCATION_IN_FILE)

/**
 * @brief  New style free
 *
 * Frees the specified memory
 *
 * By default it uses the 'MAIN' heap
 *
 * @param[out] p  The pointer to be freed
 *
 */
#define sf_mem_free(p) hm_mem_free(HEAP_MODULE_ID, p)

/**
 * @brief New style realloc
 *
 * By default it uses the 'MAIN' heap
 *
 * @param[out] *new_ptr  The pointer to allocated buffer
 * @param[in]  old_ptr   The pointer to the buffer to be reallocated
 * @param[in]  size      The size of bytes to be allocated
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
#define sf_mem_realloc(new_ptr, old_ptr, size_new) hm_mem_realloc_internal(HEAP_MODULE_ID, new_ptr, old_ptr, size_new, LOCATION_IN_FILE)

/**
 * @brief Old style malloc
 *
 * By default it uses the 'MAIN' heap
 *
 * @param[in]  size  The size of bytes to be allocated
 *
 * @return not NULL pointer if successful NULL otherwise
*/
#define sf_malloc(size) hm_malloc(HEAP_MODULE_ID, size, LOCATION_IN_FILE)

/**
 * @brief Old style
 *
 * Frees the specified memory
 *
 * By default it uses the 'MAIN' heap
 *
 * @param[out] p  The pointer to the allocated buffer
 *
 */
#define sf_free(p) hm_free(HEAP_MODULE_ID, p)

/**
 * @brief Old style realloc
 *
 * Re-allocate memory of specified size
 *
 * By default it uses the 'MAIN' heap
 *
 * @param[in]  p     The pointer to the memory to be reallocated
 * @param[in]  size  The size of bytes to be allocated
 *
 * @return not NULL pointer if successful NULL otherwise
 */
#define sf_realloc(p, size) hm_realloc(HEAP_MODULE_ID, p, size, LOCATION_IN_FILE)

/**
 * @brief Use safe memcopy when you're copying to memory allocated from the heap
 *
 * It will check whether the target pointer is valid (The target pointer has to be on the heap)
 *
 * @param[in] dst   The target pointer
 * @param[out] src  The source pointer
 * @param[in] size  The number of bytes to be copied from src to dst
 */
#define sf_memcpy(dst, src, size) hm_memcpy(HEAP_MODULE_ID, dst, src, size)

/**
 * @brief Use safe memfill when you're filling memory allocated from the heap
 *
 * It will check whether the target pointer is valid (The target pointer has to be on the heap)
 *
 * @param[in] p   The target pointer
 * @param[in] b   The byte to be used to fill the target memory
 * @param[in] size  The number of bytes to be filled
 */
#define sf_memfill(p, b, size) hm_memfill(HEAP_MODULE_ID, p, b, size)

/**
 * @brief Turn on to emulate out of memory conditions
 *
 * When emulate out of memory is turned of all
 * allocator functions will return out of memory errors
 *
 */
void mem_debug_emulate_out_of_memory(bool emulate);

/**
 * @bried Turns on memory debug run-time
 *
 * This can be useful, because at bootup debug messages might
 * freeze the system (until printf, etc are properly initalized)
 */
void mem_debug_enable_log(bool enable);

#endif
