#ifndef QFSM_H
#define QFSM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 信号类型和事件结构定义
typedef uint8_t QSignal;
typedef struct QEventTag {
    QSignal signal;    // 事件信号标识
    uint8_t dynamic;   // 动态数据标记
} QEvent;

// 状态机相关类型定义
typedef uint8_t QState;
typedef QState (*QStateHandler)(void *me, QEvent *e);  // 状态处理函数指针

// 状态机结构体
typedef struct QFsmTag {
    QStateHandler state;  // 当前状态处理函数
} QFsm;

// 工具宏定义
#define UNUSE(arg) (void)(arg)  // 未使用参数标记宏
#define QFsmCtor(me, initial) ((me)->state = (initial))  // 状态机构造宏

// 函数声明
void QFsmInit(QFsm *me, QEvent *e);      // 状态机初始化
void QFsmDispatch(QFsm *me, QEvent *e);  // 事件分发

// 状态返回值定义
#define Q_RET_HANDLED ((QState)0)  // 事件已处理
#define Q_RET_IGNORED ((QState)1)  // 事件被忽略
#define Q_RET_TRAN ((QState)2)     // 状态转换

// 状态返回宏
#define Q_HANDLED() (Q_RET_HANDLED)
#define Q_IGNORED() (Q_RET_IGNORED)
#define Q_TRAN(target) (((QFsm *)me)->state = (QStateHandler)(target), Q_RET_TRAN)

// 预定义信号枚举
enum QReservedSignals {
    Q_ENTRY_SIGNAL = 1,  // 进入状态信号
    Q_EXIT_SIGNAL,       // 退出状态信号
    Q_INIT_SIGNAL,       // 初始化信号
    Q_USER_SIGNAL,       // 用户自定义信号起始值
};

#ifdef __cplusplus
}
#endif

#endif // !QFSM_H