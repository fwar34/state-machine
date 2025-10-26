/**
 * @file sync_queue.h
 * @brief 同步队列头文件
 * 
 * 定义了线程安全的循环队列数据结构和相关操作函数，
 * 支持多线程环境下的安全入队和出队操作。
 */

#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief 同步队列结构体
 * 
 * 实现一个线程安全的循环队列，使用互斥锁和条件变量保证线程安全
 */
typedef struct {
    void **buffer;              ///< 队列缓冲区，存储指向元素的指针数组
    uint32_t head;              ///< 队列头部索引
    uint32_t tail;              ///< 队列尾部索引
    uint32_t maxSize;           ///< 队列最大容量
    uint32_t currentSize;       ///< 队列当前元素数量
    pthread_mutex_t mutex;      ///< 互斥锁，保护队列访问
    pthread_cond_t cond;        ///< 条件变量，用于线程间同步
} SyncQueue;

/**
 * @brief 初始化同步队列
 * 
 * 初始化同步队列的各个成员变量
 * 
 * @param me 指向同步队列对象的指针
 * @param buffer 用于存储队列元素的缓冲区指针数组
 * @param maxSize 队列的最大容量
 */
void QueueCtor(SyncQueue *me, void **buffer, uint32_t maxSize);

/**
 * @brief 元素入队操作
 * 
 * 将指定元素加入队列尾部，如果队列已满则返回错误
 * 
 * @param me 指向同步队列对象的指针
 * @param item 要入队的元素指针
 * @return 0 成功入队，-1 队列已满
 */
int QueueEnqueue(SyncQueue *me, void *item);

/**
 * @brief 阻塞式出队操作
 * 
 * 从队列头部取出一个元素，如果队列为空则阻塞等待直到有元素可用
 * 
 * @param me 指向同步队列对象的指针
 * @return 取出的元素指针
 */
void *QueueDequeueForever(SyncQueue *me);

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
void *QueueDequeueWithTimeout(SyncQueue *me, uint32_t timeoutMs, bool *isTimeout);

/**
 * @brief 检查队列是否为空
 * 
 * @param me 指向同步队列对象的指针
 * @return true 队列为空，false 队列不为空
 */
bool QueueIsEmpty(SyncQueue *me);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !SYNC_QUEUE_H