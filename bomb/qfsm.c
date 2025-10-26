#include "qfsm.h"

// 预定义事件数组，用于特殊信号处理
static QEvent QEP_reservedEvt[] = {
    {0, 0},              // 索引0:空事件
    {Q_ENTRY_SIGNAL, 0}, // 索引1:进入状态事件
    {Q_EXIT_SIGNAL, 0},  // 索引2:退出状态事件
    {Q_INIT_SIGNAL, 0}   // 索引3:初始化事件
};

/**
 * 初始化状态机
 * @param me 状态机实例指针
 * @param e 初始事件指针
 */
void QFsmInit(QFsm *me, QEvent *e)
{
    (me->state)(me, e);  // 调用初始状态处理函数
    (me->state)(me, &QEP_reservedEvt[Q_ENTRY_SIGNAL]);  // 发送进入状态事件
}

/**
 * 分发事件到当前状态处理函数
 * @param me 状态机实例指针
 * @param e 待处理的事件指针
 */
void QFsmDispatch(QFsm *me, QEvent *e)
{
    QStateHandler oldState = me->state;  // 保存当前状态
    QState r = oldState(me, e);          // 调用当前状态处理函数
    
    // 如果发生了状态转换
    if (r == Q_RET_TRAN) {
        oldState(me, &QEP_reservedEvt[Q_EXIT_SIGNAL]);  // 发送退出旧状态事件
        QStateHandler newState = me->state;             // 获取新状态
        newState(me, &QEP_reservedEvt[Q_ENTRY_SIGNAL]); // 发送进入新状态事件
    }
}