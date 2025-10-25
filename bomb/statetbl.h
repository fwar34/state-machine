/**
 * @file statetbl.h
 * @brief 状态表头文件
 * 
 * 定义了基于状态表的有限状态机框架的数据结构和接口，
 * 支持状态转换、事件分发等功能。
 */

#ifndef STATETBL_H
#define STATETBL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 事件结构体
 * 
 * 表示一个事件，包含信号类型标识
 */
typedef struct EventTag {
    uint16_t signal;            ///< 事件信号类型
} Event;

// 前向声明状态表结构体
struct StateTableTag;

/**
 * @brief 状态转换函数指针类型
 * 
 * 定义状态处理函数的签名
 */
typedef void (*Tran)(struct StateTableTag *me, const Event *e);

/**
 * @brief 初始状态处理函数指针类型
 * 
 * 定义初始状态处理函数的签名
 */
typedef void (*Initial)(struct StateTableTag *me);

/**
 * @brief 状态表结构体
 * 
 * 表示一个有限状态机实例，包含当前状态、状态表、状态数量、
 * 信号数量以及初始状态处理函数等信息
 */
typedef struct StateTableTag {
    uint8_t curState;           ///< 当前状态
    Tran *stateTable;           ///< 状态转换函数表（二维数组）
    uint8_t stateNum;           ///< 状态数量
    uint8_t signalNum;          ///< 信号数量
    Initial initial;            ///< 初始状态处理函数
} StateTable;

/**
 * @brief 初始化状态表对象
 * 
 * 构造函数，用于初始化状态表结构体的各个成员变量
 * 
 * @param me 指向状态表对象的指针
 * @param stateTable 状态转换函数表指针
 * @param stateNum 状态数量
 * @param signalNum 信号数量
 * @param initState 初始状态处理函数指针
 */
void StateTableCtor(StateTable *me, Tran *stateTable, uint8_t stateNum, uint8_t signalNum, Initial initState);

/**
 * @brief 初始化状态机
 * 
 * 将当前状态设置为初始状态，并调用初始状态处理函数
 * 
 * @param me 指向状态表对象的指针
 */
void StateTableInit(StateTable *me);

/**
 * @brief 分发事件到相应的状态处理函数
 * 
 * 根据当前状态和接收到的事件信号，在状态表中查找并执行相应
 * 的状态转换函数
 * 
 * @param me 指向状态表对象的指针
 * @param e 指向事件结构体的指针
 */
void StateTableDispatch(StateTable *me, const Event *e);

/**
 * @brief 空状态处理函数
 * 
 * 一个空的状态处理函数，不执行任何操作，通常用作默认处理
 * 或占位符
 * 
 * @param me 指向状态表对象的指针
 * @param e 指向事件结构体的指针
 */
void StateTableEmpty(StateTable *me, const Event *e);

/**
 * @brief 未使用参数标记宏
 * 
 * 用于消除编译器对未使用参数的警告
 */
#define UNUSE(arg) (void)(arg)

/**
 * @brief 状态转换宏
 * 
 * 用于在状态处理函数中切换到目标状态
 */
#define TRAN(target) (((StateTable *)me)->curState = (target))

#endif // !STATETBL_H