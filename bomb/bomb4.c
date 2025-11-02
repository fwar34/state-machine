#include "qfsm.h"
#include "sync_queue.h"
#include <pthread.h>
#include <conio.h>
#include <stdio.h>

// 定时器配置常量
#define BOMB_TIMOUT_INIT 15    // 初始超时时间(15秒)
#define BOMB_TIMOUT_MIN 10     // 最小超时时间(10秒)
#define BOMB_TIMOUT_MAX 120    // 最大超时时间(120秒)
#define TICK_INTERVAL_100MS 100 // 滴答间隔(100毫秒)

// 自定义事件信号定义
enum BombSignals {
    BOMB_UP_SIGNAL = Q_USER_SIGNAL,    // 增加时间信号
    BOMB_DOWN_SIGNAL,                  // 减少时间信号
    BOMB_ARM_SIGNAL,                   // 武装/解除信号
    BOMB_TICK_SIGNAL,                  // 滴答信号
};

// 扩展事件结构体，增加精细时间字段
typedef struct TickEventTag {
    QEvent super;      // 继承基础事件结构
    uint8_t fineTime;  // 精细时间计数器(0-9)
} TickEvent;

// 炸弹状态机结构体
typedef struct Bomb4Tag {
    QFsm super;        // 继承状态机基础结构
    uint8_t timeout;   // 超时倒计时
    uint8_t passwd;    // 解除密码
    uint8_t curInput;  // 当前输入序列
} Bomb4;

// 全局变量声明
static Bomb4 g_bomb4;              // 全局炸弹状态机实例
static SyncQueue keyQueue;         // 按键消息队列
static void *keyBuffer[10]; // 这里注意，一定要和syncqueue要求的数组元素类型（元素长度）匹配，
                            // 否则QueueEnqueue会给单个元素可能赋值长度更长的元素导致数组越界，
                            // 比如 static char keyBuffer[10]; QueueCtor(&keyQueue, (void **)&keyBuffer, 10);
                            // 这样QueueEnqueue会给元素数据char赋值void *指针类型的值，
                            // 最终可能会有数组越界，没有越界的时候也会导致数据数据混乱

/**
 * 显示当前超时时间
 * @param timeout 当前超时值
 */
static void DisplayTimeout(uint8_t timeout)
{
    printf("timeout[%d]\n", timeout);
}

// 前向声明状态处理函数
QState Bomb4Timing(Bomb4 *me, QEvent *e);

/**
 * 设置状态处理函数
 * 处理时间设置阶段的用户输入
 */
QState Bomb4Setting(Bomb4 *me, QEvent *e)
{
    switch (e->signal)
    {
    case Q_ENTRY_SIGNAL:
        printf("setting entry\n");
        return Q_HANDLED();
    case Q_EXIT_SIGNAL:
        printf("setting exit\n");
        return Q_HANDLED();
    case BOMB_UP_SIGNAL:
        // 增加超时时间，不超过最大值
        if (me->timeout < BOMB_TIMOUT_MAX) {
            me->timeout++;
        }
        DisplayTimeout(me->timeout);
        return Q_HANDLED();
    case BOMB_DOWN_SIGNAL:
        // 减少超时时间，不低于最小值
        if (me->timeout > BOMB_TIMOUT_MIN) {
            me->timeout--;
        }
        DisplayTimeout(me->timeout);
        return Q_HANDLED();
    case BOMB_ARM_SIGNAL:
        // 武装炸弹，进入计时状态
        me->curInput = 0;
        return Q_TRAN(Bomb4Timing);
    default:
        break;
    }

    return Q_IGNORED();
}

static bool needResetFineTime = false;  // 是否需要重置精细时间标志

/**
 * 计时状态处理函数
 * 处理炸弹倒计时和解除过程
 */
QState Bomb4Timing(Bomb4 *me, QEvent *e)
{
    switch (e->signal)
    {
    case Q_ENTRY_SIGNAL:
        needResetFineTime = true;
        printf("timing enter\n");
        return Q_HANDLED();
    case Q_EXIT_SIGNAL:
        printf("timing exit\n");
        return Q_HANDLED();
    case BOMB_UP_SIGNAL:
        // 记录输入序列: UP键对应二进制1
        me->curInput <<= 1;
        me->curInput |= 1;
        return Q_HANDLED();
    case BOMB_DOWN_SIGNAL:
        // 记录输入序列: DOWN键对应二进制0
        me->curInput <<= 1;
        return Q_HANDLED();
    case BOMB_ARM_SIGNAL:
        // 检查输入密码是否正确
        if (me->curInput == me->passwd) {
            printf("Bomb4 pause!\n");
            return Q_TRAN(Bomb4Setting);  // 密码正确，暂停炸弹
        }
        break;
    case BOMB_TICK_SIGNAL:
        // 处理滴答事件，更新倒计时
        TickEvent *te = (TickEvent *)e;
        if (te->fineTime == 0) {
            me->timeout--;
            DisplayTimeout(me->timeout);
        }

        // 时间到，炸弹爆炸并重置
        if (me->timeout == 0) {
            printf("Bomb4 bomb! Reset for again test!\n");
            me->timeout = BOMB_TIMOUT_INIT;  // 重置时间
            return Q_TRAN(Bomb4Setting);     // 返回设置状态
        }
        break;
    default:
        break;
    }

    return Q_IGNORED();
}

/**
 * 初始状态处理函数
 * 状态机启动时的初始化状态
 */
QState Bomb4Initial(Bomb4 *me, QEvent *e)
{
    UNUSE(e);
    me->timeout = BOMB_TIMOUT_INIT;  // 设置初始超时时间
    return Q_TRAN(&Bomb4Setting);    // 转换到设置状态
}

/**
 * 炸弹状态机构造函数
 * @param me 状态机实例指针
 * @param passwd 解除密码
 */
void Bomb4Ctor(Bomb4 *me, uint8_t passwd)
{
    QFsmCtor(&me->super, (QStateHandler)Bomb4Initial);  // 初始化基类
    me->passwd = passwd;  // 设置密码
}

/**
 * 炸弹控制线程函数
 * 处理用户输入和定时滴答事件
 */
void *Bomb4Run(void *arg)
{
    bool *isRunning = (bool *)arg;
    
    // 静态事件对象定义
    static QEvent upEvent = {BOMB_UP_SIGNAL, 0};
    static QEvent downEvent = {BOMB_DOWN_SIGNAL, 0};
    static QEvent armEvent = {BOMB_ARM_SIGNAL, 0};
    static TickEvent tickEvent = {{BOMB_TICK_SIGNAL, 0}, 0};

    for (;;) {
        bool isTimeout = false;
        QEvent *e = NULL;
        
        // 从按键队列获取输入，超时则产生滴答事件
        char c = (char)(uintptr_t)QueueDequeueWithTimeout(&keyQueue, TICK_INTERVAL_100MS, &isTimeout);
        if (isTimeout) {
            // 处理超时情况，生成滴答事件
            if (needResetFineTime) {
                tickEvent.fineTime = 0;
                needResetFineTime = false;
                printf("reset tickEvent.fineTime to 0!\n");
            }

            // 更新精细时间计数器
            if (++tickEvent.fineTime % 10 == 0) {
                tickEvent.fineTime = 0;
            }
            e = &tickEvent.super;
        } else {
            // 处理按键输入事件
            switch (c)
            {
            case 'u':
                e = &upEvent;
                break;
            case 'd':
                e = &downEvent;
                break;
            case 'a':
                e = &armEvent;
                break;
            case '\33':  // ESC键退出
                *isRunning = false;
                return NULL;
            default:
                break;
            }
        }

        // 分发事件到状态机
        if (e != NULL) {
            QFsmDispatch(&g_bomb4.super, e);
        }
    }

    return NULL;
}

/**
 * 主函数
 * 初始化系统并启动各组件
 */
int main()
{
    QueueCtor(&keyQueue, keyBuffer, 10);  // 初始化按键队列
    Bomb4Ctor(&g_bomb4, 0xD);             // 初始化炸弹状态机(密码0xD)
    QFsmInit(&g_bomb4.super, NULL);       // 初始化状态机

    bool isRunning = true;
    pthread_t tid;
    pthread_create(&tid, NULL, Bomb4Run, &isRunning);  // 创建控制线程

    // 主线程处理键盘输入
    while (isRunning) {
        char c = getch();  // 获取按键输入
        QueueEnqueue(&keyQueue, (void *)(uintptr_t)c);  // 加入队列
    }

    pthread_join(tid, NULL);  // 等待控制线程结束
    printf("main exit\n");

    return 0;
}