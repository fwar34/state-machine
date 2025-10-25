#include "sync_queue.h"
#include <stdio.h>

void QueueCtor(SyncQueue *me, void **buffer, uint32_t maxSize)
{
    me->buffer = buffer;
    me->head = 0;
    me->tail = 0;
    me->maxSize = maxSize;
    me->currentSize = 0;
    me->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    me->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

int QueueEnqueue(SyncQueue *me, void *item)
{
    bool isNotify = false;
    pthread_mutex_lock(&me->mutex);
    if (me->currentSize == 0) {
        isNotify = true;
    }

    if (me->currentSize == me->maxSize) {
        pthread_mutex_unlock(&me->mutex);
        return -1; // Queue full
    }
    me->buffer[me->tail] = item;
    me->tail = (me->tail + 1) % me->maxSize;
    me->currentSize++;
    if (isNotify) {
        pthread_cond_signal(&me->cond);
    }
    pthread_mutex_unlock(&me->mutex);
    return 0; // Success
}

void *QueueDequeueForever(SyncQueue *me)
{
    pthread_mutex_lock(&me->mutex);
    while (me->currentSize == 0) {
        pthread_cond_wait(&me->cond, &me->mutex);
    }
    void *item = me->buffer[me->head];
    me->head = (me->head + 1) % me->maxSize;
    me->currentSize--;
    pthread_mutex_unlock(&me->mutex);
    return item;
}

void *QueueDequeueWithTimeout(SyncQueue *me, uint32_t timeoutMs, bool *isTimeout)
{
    int ret = 0;
    struct timespec ts = {0};
    void *item = NULL;

    pthread_mutex_lock(&me->mutex);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    while (me->currentSize == 0) {
        ret = pthread_cond_timedwait(&me->cond, &me->mutex, &ts);
        if (ret == 0) {
            continue;
        } else if (ret == ETIMEDOUT) {
            *isTimeout = true;
            break;
        } else {
            // error
            printf("pthread_cond_timedwait ret[%d]\n", ret);
        }
    }

    if (ret == 0) {
        item = me->buffer[me->head];
        me->head = (me->head + 1) % me->maxSize;
        me->currentSize--;
    }
    pthread_mutex_unlock(&me->mutex);

    return item;
}

bool QueueIsEmpty(SyncQueue *me)
{
    bool ret = false;
    pthread_mutex_lock(&me->mutex);
    ret = me->currentSize == 0;
    pthread_mutex_unlock(&me->mutex);
    return ret;
}