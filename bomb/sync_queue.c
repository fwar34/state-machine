/**
 * @file sync_queue.c
 * @brief 同步队列实现文件
 * 
 * 实现了一个线程安全的循环队列，支持同步入队和出队操作，
 * 包含阻塞和带超时的出队功能。
 */

#include "sync_queue.h"
#include <stdio.h>

/**
 * @brief 初始化同步队列
 * 
 * 初始化同步队列的各个成员变量，包括缓冲区、头尾指针、
 * 大小限制以及线程同步原语
 * 
 * @param me 指向同步队列对象的指针
 * @param buffer 用于存储队列元素的缓冲区指针数组
 * @param maxSize 队列的最大容量
 */
void QueueCtor(SyncQueue *me, void **buffer, uint32_t maxSize)
{
    // 设置队列缓冲区
    me->buffer = buffer;
    // 初始化队列头指针
    me->head = 0;
    // 初始化队列尾指针
    me->tail = 0;
    // 设置队列最大容量
    me->maxSize = maxSize;
    // 初始化当前队列大小
    me->currentSize = 0;
    // 初始化互斥锁
    me->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    // 初始化条件变量
    me->cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
}

/**
 * @brief 元素入队操作
 * 
 * 将指定元素加入队列尾部，如果队列已满则返回错误
 * 
 * @param me 指向同步队列对象的指针
 * @param item 要入队的元素指针
 * @return 0 成功入队，-1 队列已满
 */
int QueueEnqueue(SyncQueue *me, void *item)
{
    bool isNotify = false;
    // 加锁保护临界区
    pthread_mutex_lock(&me->mutex);
    
    // 如果队列为空，需要通知等待的消费者线程
    if (me->currentSize == 0) {
        isNotify = true;
    }

    // 检查队列是否已满
    if (me->currentSize == me->maxSize) {
        // 解锁并返回错误
        pthread_mutex_unlock(&me->mutex);
        return -1; // Queue full
    }
    
    // 将元素放入队列尾部
    me->buffer[me->tail] = item;
    // 更新尾指针（循环队列）
    me->tail = (me->tail + 1) % me->maxSize;
    // 增加当前队列大小
    me->currentSize++;
    
    // 如果之前队列为空，唤醒等待的线程
    if (isNotify) {
        pthread_cond_signal(&me->cond);
    }
    
    // 解锁
    pthread_mutex_unlock(&me->mutex);
    return 0; // Success
}

/**
 * @brief 阻塞式出队操作
 * 
 * 从队列头部取出一个元素，如果队列为空则阻塞等待直到有元素可用
 * 
 * @param me 指向同步队列对象的指针
 * @return 取出的元素指针
 */
void *QueueDequeueForever(SyncQueue *me)
{
    // 加锁保护临界区
    pthread_mutex_lock(&me->mutex);
    
    // 如果队列为空，则等待直到有元素
    while (me->currentSize == 0) {
        pthread_cond_wait(&me->cond, &me->mutex);
    }
    
    // 取出队列头部元素
    void *item = me->buffer[me->head];
    // 更新头指针（循环队列）
    me->head = (me->head + 1) % me->maxSize;
    // 减少当前队列大小
    me->currentSize--;
    
    // 解锁
    pthread_mutex_unlock(&me->mutex);
    return item;
}

/**
 * @brief 带超时的出队操作
 * 
 * 从队列头部取出一个元素，如果队列为空则等待指定时间，
 * 超时后返回NULL
 * 
 * @param me 指向同步队列对象的指针
 * @param timeoutMs 超时时间（毫秒）
 * @param isTimeout 输出参数，标识是否超时
 * @return 取出的元素指针，超时或失败时返回NULL
 */
void *QueueDequeueWithTimeout(SyncQueue *me, uint32_t timeoutMs, bool *isTimeout)
{
    int ret = 0;
    struct timespec ts = {0};
    void *item = NULL;

    // 加锁保护临界区
    pthread_mutex_lock(&me->mutex);
    
    // 获取当前时间并计算超时时间点
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    
    // 如果队列为空，则等待直到有元素或超时
    while (me->currentSize == 0) {
        ret = pthread_cond_timedwait(&me->cond, &me->mutex, &ts);
        if (ret == 0) {
            // 成功被唤醒，继续检查队列状态
            continue;
        } else if (ret == ETIMEDOUT) {
            // 超时，设置超时标志并退出循环
            *isTimeout = true;
            break;
        } else {
            // 其他错误情况
            printf("pthread_cond_timedwait ret[%d]\n", ret);
        }
    }

    // 如果成功获取到元素
    if (ret == 0) {
        item = me->buffer[me->head];
        me->head = (me->head + 1) % me->maxSize;
        me->currentSize--;
    }
    
    // 解锁
    pthread_mutex_unlock(&me->mutex);

    return item;
}

/**
 * @brief 检查队列是否为空
 * 
 * @param me 指向同步队列对象的指针
 * @return true 队列为空，false 队列不为空
 */
bool QueueIsEmpty(SyncQueue *me)
{
    bool ret = false;
    // 加锁保护临界区
    pthread_mutex_lock(&me->mutex);
    // 检查队列大小是否为0
    ret = me->currentSize == 0;
    // 解锁
    pthread_mutex_unlock(&me->mutex);
    return ret;
}