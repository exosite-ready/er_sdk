/**
 * @file concurrent_queue.c
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
 * A concurrent queue implementation
 **/
#include <lib/concurrent_queue.h>
#include <lib/sf_malloc.h>
#include <porting/thread_port.h>
#include <lib/error.h>
#include <lib/debug.h>

struct _CQueue {
    Queue *queue;
    system_mutex_t queue_lock;
    size_t max_size;
};

CQueue *cqueue_new(size_t max_size)
{
    CQueue *cqueue;

    cqueue = (CQueue *) sf_malloc(sizeof(CQueue));
    if (cqueue == NULL)
        return NULL;

    cqueue->queue = queue_new();
    if (cqueue->queue == NULL)
        goto cleanUp1;

    if (system_mutex_create(&cqueue->queue_lock))
        goto cleanUp2;

    cqueue->max_size = max_size;

    return cqueue;

cleanUp2:
    queue_free(cqueue->queue);
cleanUp1:
    sf_free(cqueue);
    return NULL;
}

void cqueue_free(CQueue *cqueue)
{
    ASSERT(cqueue);

    system_mutex_destroy(cqueue->queue_lock);
    queue_free(cqueue->queue);
    sf_free(cqueue);
}

size_t cqueue_get_size(CQueue *cqueue)
{
    size_t ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_get_size(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

int32_t cqueue_push_head(CQueue *cqueue, CQueueValue data)
{
    int32_t ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_push_head(cqueue->queue, data);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

CQueueValue cqueue_pop_head(CQueue *cqueue)
{
    CQueueValue ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_pop_head(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

CQueueValue cqueue_peek_head(CQueue *cqueue)
{
    CQueueValue ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_peek_head(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

int32_t cqueue_push_tail(CQueue *cqueue, CQueueValue data)
{
    int32_t ret = ERR_FAILURE;
    size_t size;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);

    size = queue_get_size(cqueue->queue);
    if (size < cqueue->max_size)
        ret = queue_push_tail(cqueue->queue, data);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

CQueueValue cqueue_pop_tail(CQueue *cqueue)
{
    CQueueValue ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_pop_tail(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

CQueueValue cqueue_peek_tail(CQueue *cqueue)
{
    CQueueValue ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_peek_tail(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

int32_t cqueue_is_empty(CQueue *cqueue)
{
    int32_t ret;

    ASSERT(cqueue);
    system_mutex_lock(cqueue->queue_lock);
    ret = queue_is_empty(cqueue->queue);
    system_mutex_unlock(cqueue->queue_lock);

    return ret;
}

