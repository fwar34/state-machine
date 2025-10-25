/**
 * @file bomb2.c
 * @brief 炸弹定时器状态机实现
 * 
 * 实现了一个炸弹定时器模拟程序，使用状态表驱动的有限状态机，
 * 包含设置和计时两个主要状态，支持密码解锁功能。
 */

#include "statetbl.h"
#include "sync_queue.h"
#include <stdio.h>
#include <unistd.h>
#include <conio.h>
#include <pthread.h>

// 定义各种常量
#define BOMB2_INIT_TIMEOUT 15   ///< 初始超时时间（秒）
#define BOMB2_MIN_TIMEOUT 10    ///< 最小超时时间（秒）
#define BOMB2_MAX_TIMEOUT 120   ///< 最大超时时间（秒）
#define TICK_INTERVAL_100MS 100 ///< 滴答间隔（毫秒）

/**
 * @brief 炸弹状态枚举
 */
typedef enum {
    BOMB_STATE_SETTING,         ///< 设置状态
    BOMB_STATE_TIMING,          ///< 计时状态
    BOMB_STATE_MAX,             ///< 状态数量
} BombState;

/**
 * @brief 炸弹信号枚举
 */
typedef enum {
    BOMB_SIGNAL_UP,             ///< 增加时间信号
    BOMB_SIGNAL_DOWN,           ///< 减少时间信号
    BOMB_SIGNAL_ARM,            ///< 启动/停止信号
    BOMB_SIGNAL_TICK,           ///< 时间滴答信号
    BOMB_SIGNAL_MAX,            ///< 信号数量
} BombSignal;

// 状态和信号的数量定义
#define STATE_NUM 2             ///< 状态数量
#define SIGNAL_NUM 4            ///< 信号数量

/**
 * @brief 炸弹定时器结构体
 * 
 * 继承自 StateTable，扩展了炸弹特定的属性
 */
typedef struct Bomb2Tag {
    StateTable super;           ///< 继承的状态表基类
    uint32_t timeout;           ///< 超时时间（秒）
    uint8_t passwd;             ///< 解锁密码
    uint8_t curInput;           ///< 当前输入的密码
} Bomb2;

static Bomb2 g_bomb2;           ///< 全局炸弹对象实例

/**
 * @brief 滴答事件结构体
 * 
 * 扩展基础事件结构，增加精细时间信息
 */
typedef struct TickEventTag {
    Event super;                ///< 继承的基础事件
    uint16_t fineTime;          ///< 精细时间计数器（0-9）
} TickEvent;

/**
 * @brief 显示当前超时时间
 * 
 * @param timeout 当前超时时间值
 */
static void DisplayTimeout(uint32_t timeout)
{
    printf("curTimeout[%d]\n", timeout);
}

/**
 * @brief 显示当前输入密码
 * 
 * @param c 输入字符（'u'或'd'）
 * @param curInput 当前累积的输入值
 */
static void DisplayCurInput(const char c, uint8_t curInput)
{
    printf("%c, curInput[0x%02x]\n", c, curInput);
}

/**
 * @brief 设置状态下处理UP信号
 * 
 * 增加超时时间（不超过最大值）
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombSettingUp(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    // 如果未达到最大超时时间，则增加
    if (me->timeout < BOMB2_MAX_TIMEOUT) {
        me->timeout++;
    }
    DisplayTimeout(me->timeout);
}

/**
 * @brief 设置状态下处理DOWN信号
 * 
 * 减少超时时间（不低于最小值）
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombSettingDown(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    // 如果未达到最小超时时间，则减少
    if (me->timeout > BOMB2_MIN_TIMEOUT) {
        me->timeout--;
    }
    DisplayTimeout(me->timeout);
}

/**
 * @brief 设置状态下处理ARM信号
 * 
 * 启动炸弹计时器，进入计时状态
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombSettingArm(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    printf("Bomb2 start\n");
    DisplayTimeout(me->timeout);
    // 清空当前输入
    me->curInput = 0;
    // 转换到计时状态
    TRAN(BOMB_STATE_TIMING);
}

/**
 * @brief 计时状态下处理UP信号
 * 
 * 处理密码输入的"UP"按键
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombTimingUp(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    // 左移一位并在最低位设置为1
    me->curInput <<= 1;
    me->curInput |= 1;
    DisplayCurInput('u', me->curInput);
}

/**
 * @brief 计时状态下处理DOWN信号
 * 
 * 处理密码输入的"DOWN"按键
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombTimingDown(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    // 左移一位，最低位保持为0
    me->curInput <<= 1;
    DisplayCurInput('d', me->curInput);
}

/**
 * @brief 计时状态下处理ARM信号
 * 
 * 停止计时器并验证密码
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向事件的指针
 */
void BombTimingArm(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    printf("Bomb2 stop, curInput[0x%02x]\n", me->curInput);
    // 验证输入密码是否正确
    if (me->curInput == me->passwd) {
        me->curInput = 0;
        // 密码正确，回到设置状态
        TRAN(BOMB_STATE_SETTING);
    }
    DisplayTimeout(me->timeout);
}

/**
 * @brief 计时状态下处理TICK信号
 * 
 * 处理时间滴答事件，更新倒计时
 * 
 * @param me 指向炸弹对象的指针
 * @param e 指向滴答事件的指针
 */
void BombTimingTick(Bomb2 *me, const TickEvent *e)
{
    // 检查超时时间是否有效
    if (me->timeout == 0) {
        // 错误情况：超时时间为0
        printf("Timing: Tick, error fineTime:%d\n", e->fineTime);
        return;
    }

    // 每秒更新一次倒计时（10个100ms滴答等于1秒）
    if (e->fineTime == 0) {
        me->timeout--;
        DisplayTimeout(me->timeout);
    }
    
    // 如果倒计时结束，触发爆炸
    if (me->timeout == 0) {
        printf("Bomb2 bomb!!! Reset for again test!\n");
        // 重置超时时间为初始值
        me->timeout = BOMB2_INIT_TIMEOUT;
        // 回到设置状态
        TRAN(BOMB_STATE_SETTING);
    }
}

// 定义状态表：二维数组[state][signal]
static Tran stateTable[STATE_NUM][SIGNAL_NUM] = {
    // 设置状态下的各信号处理函数
    {(Tran)BombSettingUp, (Tran)BombSettingDown, (Tran)BombSettingArm, StateTableEmpty},
    // 计时状态下的各信号处理函数
    {(Tran)BombTimingUp, (Tran)BombTimingDown, (Tran)BombTimingArm, (Tran)BombTimingTick},
};

// 键盘输入队列及相关变量
static SyncQueue keyQueue;              ///< 键盘输入队列
static void *keyBuffer[10];             ///< 队列缓冲区

/**
 * @brief 炸弹初始状态处理函数
 * 
 * 初始化炸弹的各项