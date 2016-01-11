/**
 * @file concurrent_queue.h
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
 * @brief Interface for the concurrent queue
 **/
#ifndef INC_CONCURRENT_QUEUE_H_
#define INC_CONCURRENT_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lib/type.h>
#include <lib/queue.h>


/* A double-ended concurrent queue. */
typedef struct _CQueue CQueue;

/* A value stored in a CQueue. */
typedef QueueValue CQueueValue;

#define QUEUE_SIZE_UNLIMITED 0xffffffff
/**
 * Create a new double-ended queue.
 *
 * @return A new queue, or NULL if it was not possible to allocate
 *         the memory.
 */
CQueue *cqueue_new(size_t max_size);

/**
 * Destroy a queue.
 *
 * @param [in] queue      The queue to destroy.
*/
void cqueue_free(CQueue *cqueue);

/**
 * Get item number in the queue
 *
 * @param [in] queue      The queue
 *
 * @return Number of the items stored in the queue.
 */
size_t cqueue_get_size(CQueue *cqueue);

/**
 * Add a value to the head of a queue.
 *
 * @param [in] queue    The queue.
 * @param [in] data     The value to add.
 *
 * @return error code (0 for success, negative number otherwise)
 */
int32_t cqueue_push_head(CQueue *cqueue, CQueueValue data);

/**
 * Remove a value from the head of a queue.
 *
 * @param [in] queue   The queue.
 *
 * @return Value that was at the head of the queue, or
 *         QUEUE_NULL if the queue is empty.
 */
CQueueValue cqueue_pop_head(CQueue *cqueue);

/**
 * Read value from the head of a queue, without removing it from
 * the queue.
 *
 * @param [in] queue      The queue.
 *
 * @return Value at the head of the queue, or QUEUE_NULL
 *         if the  queue is empty.
 */
CQueueValue cqueue_peek_head(CQueue *cqueue);

/**
 * Add a value to the tail of a queue.
 *
 * @param [in] queue      The queue.
 * @param [in] data       The value to add.
 *
 * @return error code (0 for success, negative number otherwise)
 */
int32_t cqueue_push_tail(CQueue *cqueue, CQueueValue data);

/**
 * Remove a value from the tail of a queue.
 *
 * @param [in] queue      The queue.
 *
 * @return Value that was at the head of the queue, or
 *         QUEUE_NULL if the queue is empty.
 */
CQueueValue cqueue_pop_tail(CQueue *cqueue);

/**
 * Read a value from the tail of a queue, without removing it from
 * the queue.
 *
 * @param [in] queue      The queue.
 *
 * @return Value at the tail of the queue, or QUEUE_NULL if the
 *         queue is empty.
 */
CQueueValue cqueue_peek_tail(CQueue *cqueue);

/**
 * Query if any values are currently in a queue.
 *
 * @param [in] queue      The queue.
 *
 * @return Zero if the queue is not empty, non-zero if the queue
 *         is empty.
 */
int32_t cqueue_is_empty(CQueue *cqueue);

#ifdef __cplusplus
}
#endif

#endif /* INC_CONCURRENT_QUEUE_H_ */
