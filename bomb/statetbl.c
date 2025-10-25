/**
 * @file statetbl.c
 * @brief 状态表实现文件
 * 
 * 该文件实现了基于状态表的有限状态机框架，
 * 包括状态机的构造、初始化、事件分发等功能。
 */

#include "statetbl.h"

/**
 * @brief 初始化状态表对象
 * 
 * 构造函数，用于初始化状态表结构体的各个成员变量
 * 
 * @param me 指向状态表对象的指针
 * @param stateTable 状态转换函数表指针
 * @param stateNum 状态数量
 * @param signalNum 信号数量
 * @param Initial 初始状态处理函数指针
 */
void StateTableCtor(StateTable *me, Tran *stateTable, uint8_t stateNum, uint8_t signalNum, Initial Initial)
{
    // 设置状态转换表
    me->stateTable = stateTable;
    // 设置状态数量
    me->stateNum = stateNum;
    // 设置信号数量
    me->signalNum = signalNum;
    // 设置初始状态处理函数
    me->initial = Initial;
}

/**
 * @brief 初始化状态机
 * 
 * 将当前状态设置为初始状态，并调用初始状态处理函数
 * 
 * @param me 指向状态表对象的指针
 */
void StateTableInit(StateTable *me)
{
    // 将当前状态设置为状态数（作为初始标记）
    me->curState = me->stateNum;
    // 调用初始状态处理函数
    (me->initial)(me);
}

/**
 * @brief 分发事件到相应的状态处理函数
 * 
 * 根据当前状态和接收到的事件信号，在状态表中查找并执行相应
 * 的状态转换函数
 * 
 * @param me 指向状态表对象的指针
 * @param e 指向事件结构体的指针
 */
void StateTableDispatch(StateTable *me, const Event *e)
{
    // 检查事件信号是否超出范围
    if (e->signal >= me->signalNum) {
        // 信号超出范围，直接返回
        return;
    }

    // 计算状态转换表中的索引并调用相应的转换函数
    // 索引计算公式: currentState * signalNum + signal
    me->stateTable[me->curState * me->signalNum + e->signal](me, e);

    // 检查状态转换后当前状态是否合法
    if (me->curState >= me->stateNum) {
        // 当前状态超出有效范围，发生错误
        // 错误处理逻辑应在此处添加
    }
}

/**
 * @brief 空状态处理函数
 * 
 * 一个空的状态处理函数，不执行任何操作，通常用作默认处理
 * 或占位符
 * 
 * @param me 指向状态表对象的指针
 * @param e 指向事件结构体的指针
 */
void StateTableEmpty(StateTable *me, const Event *e)
{
    // 使用UNUSE宏避免未使用参数警告
    UNUSE(me);
    UNUSE(e);
}