/**
 * @file heap_manager.h
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
 * @brief Heap manager allocator interface; this interface routes the
 *        allocator calls based on the HEAP ID
 *
 * Features:
 *   - Support for several heaps
 *   - Thread safe
 *   - In debug mode instrumentation is added
 *   - Support for memory IDs
 *
 **/
#ifndef ER_SDK_SRC_INCLUDE_LIB_HEAP_MANAGER_H_
#define ER_SDK_SRC_INCLUDE_LIB_HEAP_MANAGER_H_

/**
 * @brief New style hm allocator
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[out] *ptr       The pointer to the allocated buffer
 * @param[in]  size       The size of bytes to be allocated
 * @param[in]  chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
int32_t hm_mem_alloc(int32_t heap_id, void **ptr, size_t size, const char *chunk_id);

/**
 * @brief New style hm reallocator
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[out] *new_ptr   The pointer to the allocated buffer. It is only overwritten
 *                        if the allocation was successful
 * @param[in]  old_ptr    The pointer to the buffer to be reallocated
 * @param[in]  sizeNew    The size of bytes to be allocated
 * @param[in]  chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
int32_t hm_mem_realloc(int32_t heap_id, void **new_ptr, void *old_ptr, size_t size_new, const char *chunk_id);

/**
 * @brief New style free
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[int] *ptr        The pointer to be freed
 *
 * @return ERR_SUCCESS if successful ERR_NO_MEMORY otherwise
 */
void hm_mem_free(int32_t heap_id, void *ptr);

/**
 * @brief Old style hm allocator
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[in]  size       The size of bytes to be allocated
 * @param[in]  chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 * @return pointer to the allocated memory when succeeded or NULL otherwise
 */
void *hm_malloc(int32_t heap_id, size_t size, const char *id);

/**
 * @brief Old style hm reallocator
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[in]  p          The pointer to be reallocated
 * @param[in]  size       The size of bytes to be reallocated
 * @param[in]  chunk_id   An optional id for the allocated memory, if it is NULL it is
 *                        not used; otherwise it is saved to the instrumentations structure
 *
 * @return pointer to the allocated memory when succeeded or NULL otherwise
 */
void *hm_realloc(int32_t heap_id, void *p, size_t size, const char *id);

/**
 * @brief Old style hm free
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[in]  p          The pointer to be freed
 *
 * @return pointer to the allocated memory when succeeded or NULL otherwise
 */
void hm_free(int32_t heap_id, void *p);

/**
 * @brief Use hm_memcopy when you're copying to memory allocated from the heap
 *
 * This check only works if HEAP_TRACKER_ENABLED is defined
 * It will check whether the target pointer is valid (The target pointer has to be on the heap)
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[in]  dst        The target pointer
 * @param[out] src        The source pointer
 * @param[in] size        The number of bytes to be copied from src to dst
 */
void hm_memcpy(int32_t heap_id, void *dst, const void *src, size_t size);

/**
 * @brief Use hm_memfill when you're filling memory allocated from the heap
 *
 * This check only works if HEAP_TRACKER_ENABLED is defined
 * It will check whether the target pointer is valid (The target pointer has to be on the heap)
 *
 * @param[in]  heap_id    It uses the heap with this memory ID to allocate memory from
 * @param[in] p           The target pointer
 * @param[in] b           The byte to be used to fill the target memory
 * @param[in] size        The number of bytes to be filled
 */
void hm_memfill(int32_t heap_id, void *p, uint8_t b, size_t size);

/**
 * @brief Initalize the heap manager
 *
 * It will check whether the target pointer is valid (The target pointer has to be on the heap)
 *
 * @param[in] base    Optional; if it is not NULL then the heap manager will use this memory area for allocating
 * @param[in] size    Optional; it should be set if base is not NULL
 *
 * @return ERR_SUCESS on success ERR_* error code otherwise
 */
int32_t hm_init(void *base, size_t size);



#endif /* ER_SDK_SRC_INCLUDE_LIB_HEAP_MANAGER_H_ */
