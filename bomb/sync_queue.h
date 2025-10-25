#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <stdint.h>
#include <pthread.h>

typedef struct {
    void **buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t maxSize;
    uint32_t currentSize;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SyncQueue;

void QueueCtor(SyncQueue *me, void **buffer, uint32_t maxSize);
int QueueEnqueue(SyncQueue *me, void *item);
void *QueueDequeueForever(SyncQueue *me);
void *QueueDequeueWithTimeout(SyncQueue *me, uint32_t timeoutMs, bool *isTimeout);
bool QueueIsEmpty(SyncQueue *me);

#endif // !SYNC_QUEUE_H