/**
 * @file hs_mem.h
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
 * This file provides the memory allocator interface
 *
 * This interface allows the construction of heap-spaces and provides
 * a malloc-like interface to it;
 * The differences are:
 *   - an additional heap object parameter which defines which heap has to be used
 *   - an additional id parameter which can be optionally provided
 *
 * A heap space is an abstraction of the HEAP; it can be
 *   - a normal HEAP accessed by standard malloc/free, etc calls
 *   - it can be a normal HEAP accessed by registered malloc/free, etc calls
 *   - it can be a memory space which uses an internal allocator
 *
 * This interface hides details from the heap_manager; the heap manager will use this interface
 * and request memory through this interface
 **/

#ifndef ER_SDK_SRC_LIB_HS_MEM_H_
#define ER_SDK_SRC_LIB_HS_MEM_H_

/**
 *  @brief Construct a heap_space object based on the heap base pointer and size
 *
 *  @param[in] heap   The heap_space object which will be initalized by this call; it has
 *                    to be allocated by the callee
 *  @param[in] base   The pointer to the base of the heap
 *  @param[in] size   The size of the heap
 *
 *  @return ERR_SUCCESS if succeeded ERR_* error code otherwise
 */
int32_t hs_construct(struct heap_space_class *heap, void *base, size_t size, const char *name, int32_t id);

/**
 *  @brief Allocates memory from the given heap
 *
 * @param[in]  heap   The heap_space object from which memory is allocated
 * @param[out] *ptr       The pointer to the allocated buffer
 * @param[in]  size       The size of bytes to be allocated
 * @param[in]  chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 *  @return ERR_SUCCESS on success ERR_* error code otherwise
 */
int32_t hs_mem_alloc(struct heap_space_class *heap, void **ptr, size_t size, const char *id);

/**
 *  @brief Reallocates memory from the given heap
 *
 * @param[in]      heap   The heap_space object from which memory is reallocated
 * @param[out]     *new_ptr   The pointer to the allocated buffer. It is only overwritten
 *                           if the allocation was successful
 * @param[in]      old_ptr   The pointer to the memory chunk to be reallocated
 * @param[in]      size_new    The size of bytes to be allocated
 * @param[in]      chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
int32_t hs_mem_realloc(struct heap_space_class *heap, void **new_ptr, void *old_ptr, size_t size_new, const char *id);

/**
 *  @brief Frees memory from the given heap
 *
 * @param[in] heap   The heap_space object in which memory is freed
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[int] ptr        The pointer to be freed
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
void hs_mem_free(struct heap_space_class *heap, void *ptr);

/**
 *  @brief Prints information about the given heap
 *
 *  @param[in] heap   The heap_space object
 *
 */
void hs_get_stats(struct heap_space_class *heap);



#endif /* ER_SDK_SRC_LIB_HS_MEM_H_ */
