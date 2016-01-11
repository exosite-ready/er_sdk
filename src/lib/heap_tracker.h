/**
 * @file heap_tracker.h
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
 * This file provides the heap tracker interface
 *
 * This interface provides:
 *    - functions to add/remove/update memory blocks to/from/in the tracking logs
 *    - functions to prefill/postfill/wrap memory blocks
 *    - function to validate a pointer
 *
 *
 *
 **/

#ifndef ER_SDK_SRC_LIB_HEAP_TRACKER_H_
#define ER_SDK_SRC_LIB_HEAP_TRACKER_H_

#ifdef HEAP_TRACKER_ENABLED
int32_t heap_tracker_init(struct heap_space_class *self);
int32_t heap_tracker_add_block(struct heap_space_class *self, void *p, size_t size, const char *id);
int32_t heap_tracker_remove_block(struct heap_space_class *self, void *p);
int32_t heap_tracker_update_block(struct heap_space_class *self, void *p, size_t new_size, const char *id);
BOOL heap_tracker_is_pointer_valid(struct heap_space_class *self, void *p);
BOOL heap_tracker_is_range_valid(struct heap_space_class *self, void *p, size_t size);
void heap_tracker_print_mem_blocks(struct heap_space_class *self);
void heap_tracker_print_allocated_memory(struct heap_space_class *self);
int32_t heap_tracker_get_usage(struct heap_space_class *self);
void heap_tracker_check_memory(struct heap_space_class *self);
void *heap_tracker_malloc(struct heap_space_class *self, size_t size);
void heap_tracker_free(struct heap_space_class *self, void *p);
void *heap_tracker_realloc(struct heap_space_class *self, void *p, size_t new_size);
#else

static inline int32_t heap_tracker_init(struct heap_space_class *self)
{
    (void)self;
    return ERR_SUCCESS;
}

static inline int32_t heap_tracker_add_block(struct heap_space_class *self, void *p, size_t size, const char *id)
{
    (void)self;
    (void)p;
    (void)size;
    (void)id;
    return ERR_SUCCESS;
}

static inline int32_t heap_tracker_remove_block(struct heap_space_class *self, void *p)
{
    (void)self;
    (void)p;
    return ERR_SUCCESS;
}

static inline int32_t heap_tracker_update_block(struct heap_space_class *self, void *p, size_t new_size, const char *id)
{
    (void)self;
    (void)p;
    (void)new_size;
    (void)id;
    return ERR_SUCCESS;
}

static inline BOOL heap_tracker_is_pointer_valid(struct heap_space_class *self, void *p)
{
    (void)self;
    (void)p;
    return TRUE;
}

static inline BOOL heap_tracker_is_range_valid(struct heap_space_class *self, void *p, size_t size)
{
    (void)self;
    (void)p;
    (void)size;
    return TRUE;
}

static inline void heap_tracker_print_mem_blocks(struct heap_space_class *self)
{
    (void)self;
}

static inline void heap_tracker_print_allocated_memory(struct heap_space_class *self)
{
    (void)self;
}

static inline int32_t heap_tracker_get_usage(struct heap_space_class *self)
{
    (void)self;
    return 0;
}

static inline void heap_tracker_check_memory(struct heap_space_class *self)
{
    (void)self;
}
#endif

#endif /* ER_SDK_SRC_LIB_HEAP_TRACKER_H_ */
