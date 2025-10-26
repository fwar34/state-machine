#include <iostream>
#include <cstdint>
#include <thread>
#include <array>
#include <functional>
#include <variant>
#include <string>
#include <conio.h>
#include "sync_queue.h"

// 定义初始超时时间（秒）
constexpr uint8_t TIMEOUT_INITIAL = 15U;
// 定义最小超时时间（秒）
constexpr uint8_t TIMEOUT_MIN = 10U;
// 定义最大超时时间（秒）
constexpr uint8_t TIMEOUT_MAX = 120U;
// 定义定时器周期（毫秒）
constexpr uint32_t TICK100MS = 100;
// 子状态数量
constexpr uint8_t SUB_STATE_NUM = 4;
// 退出状态标识
constexpr uint8_t STATE_EXIT = 255;

// 子状态枚举
enum SubState : uint8_t
{
    SUB_STATE_UP = 0,    // 向上调整
    SUB_STATE_DOWN,      // 向下调整
    SUB_STATE_ARM,       // 武器激活
    SUB_STATE_TICK,      // 计时器滴答
    SUB_STATE_MAX
};

// 前向声明Bomb3类
class Bomb3;

// 状态基类，定义状态接口
class BombState
{
public:
    // 纯虚函数，处理向上操作
    virtual void OnUp(Bomb3 *bomb) = 0;
    // 纯虚函数，处理向下操作
    virtual void OnDown(Bomb3 *bomb) = 0;
    // 纯虚函数，处理武器激活操作
    virtual void OnArm(Bomb3 *bomb) = 0;
    // 纯虚函数，处理计时器滴答
    virtual void OnTick(Bomb3 *bomb, uint8_t fineTime) = 0;
};

// 设置状态类，继承自BombState
class SettingState final : public BombState
{
public:
    void OnUp(Bomb3 *bomb) override;
    void OnDown(Bomb3 *bomb) override;
    void OnArm(Bomb3 *bomb) override;
    void OnTick(Bomb3 *bomb, uint8_t fineTime) override;
};

// 计时状态类，继承自BombState
class TimingState final : public BombState
{
public:
    void OnUp(Bomb3 *bomb) override;
    void OnDown(Bomb3 *bomb) override;
    void OnArm(Bomb3 *bomb) override;
    void OnTick(Bomb3 *bomb, uint8_t fineTime) override;
};

// 键盘输入队列及相关变量
static SyncQueue keyQueue;
static void *keyBuffer[10] = {0};             ///< 队列缓冲区

// 主要的炸弹控制类
class Bomb3
{
public:
    // 初始化函数
    void Init(uint8_t passwd)
    {
        curState_ = &setting_;  // 初始状态为设置状态
        timeout_ = TIMEOUT_INITIAL;  // 设置初始超时时间
        passwd_ = passwd;  // 设置密码
        curInput_ = 0;  // 初始化当前输入

        // 初始化子状态处理函数表
        subStateTable_[SubState::SUB_STATE_UP] = [this] {
            OnUp();
        };
        subStateTable_[SubState::SUB_STATE_DOWN] = [this] {
            OnDown();
        };
        subStateTable_[SubState::SUB_STATE_ARM] = [this] {
            OnArm();
        };
        subStateTable_[SubState::SUB_STATE_TICK] = [this] (uint8_t fineTime) {
            OnTick(fineTime);
        };
    }

    // 处理向上操作
    void OnUp()
    {
        curState_->OnUp(this);
    }

    // 处理向下操作
    void OnDown()
    {
        curState_->OnDown(this);
    }

    // 处理武器激活操作
    void OnArm()
    {
        curState_->OnArm(this);
    }

    // 处理计时器滴答
    void OnTick(uint8_t fineTime)
    {
        if (curState_ == &timing_) {
            curState_->OnTick(this, fineTime);
        }
    }

    // 主运行循环
    void Run()
    {
        for (;;) {
            bool isTimeout = false;
            // 从队列中取出状态或等待超时
            uint8_t state = (uint8_t)(uintptr_t)QueueDequeueWithTimeout(&keyQueue, TICK100MS, &isTimeout);
            if (isTimeout) {
                // 处理计时器滴答
                static uint8_t fineTime = 0;
                if (++fineTime == 10) {
                    fineTime = 0;
                }
                // 调用滴答处理函数
                if (auto func = std::get_if<std::function<void(uint8_t)>>(&subStateTable_[SubState::SUB_STATE_TICK])) {
                    (*func)(fineTime);
                } else {
                    std::cout << "error type function" << std::endl;
                }
            } else if (state == STATE_EXIT) {
                // 处理退出状态
                break;
            } else {
                // 处理其他状态
                if (auto func = std::get_if<std::function<void()>>(&subStateTable_[state])) {
                    (*func)();
                }
            }
        }
    }

private:
    // 状态转换函数
    void Tran(BombState *state)
    {
        curState_ = state;
    }

private:
    BombState *curState_;  // 当前状态
    uint8_t timeout_;      // 超时时间
    uint8_t passwd_;       // 密码
    uint8_t curInput_;     // 当前输入

    // 子状态函数类型定义
    using SubStateFunction = std::variant<
        std::function<void()>,
        std::function<void(uint8_t)>
    >;
    
    // 子状态处理函数表
    std::array<SubStateFunction, SUB_STATE_NUM> subStateTable_;

    // 静态状态实例
    static SettingState setting_;
    static TimingState timing_;

    // 友元类声明
    friend class SettingState;
    friend class TimingState;
};

// 静态成员初始化
SettingState Bomb3::setting_;
TimingState Bomb3::timing_;

// 打印超时信息的辅助函数
static void PrintTimeout(const std::string &s, uint8_t timeout)
{
    std::cout << s << ", Bomb3 timeout[" << timeout << "]" << std::endl;
}

// 设置状态下处理向上操作
void SettingState::OnUp(Bomb3 *bomb)
{
    if (bomb->timeout_ < TIMEOUT_MAX) {
        bomb->timeout_++;
    }
    PrintTimeout("u", bomb->timeout_);
}

// 设置状态下处理向下操作
void SettingState::OnDown(Bomb3 *bomb)
{
    if (bomb->timeout_ > TIMEOUT_MIN) {
        bomb->timeout_--;
    }
    PrintTimeout("d", bomb->timeout_);
}

// 设置状态下处理武器激活操作
void SettingState::OnArm(Bomb3 *bomb)
{
    bomb->Tran(&Bomb3::timing_);
    bomb->curInput_ = 0;
    std::cout << "Bomb3 start..." << std::endl;
}

// 设置状态下处理计时器滴答（空实现）
void SettingState::OnTick(Bomb3 *bomb, uint8_t fineTime)
{
    (void)bomb;
    (void)fineTime;
}

// 计时状态下处理向上操作
void TimingState::OnUp(Bomb3 *bomb)
{
    bomb->curInput_ <<= 1;
    bomb->curInput_ |= 1;
    std::cout << "u, curInput[" << bomb->curInput_ << "]" << std::endl;
}

// 计时状态下处理向下操作
void TimingState::OnDown(Bomb3 *bomb)
{
    bomb->curInput_ <<= 1;
    std::cout << "d, curInput[" << bomb->curInput_ << "]" << std::endl;
}

// 计时状态下处理武器激活操作
void TimingState::OnArm(Bomb3 *bomb)
{
    if (bomb->curInput_ == bomb->passwd_) {
        bomb->Tran(&Bomb3::setting_);
        std::cout << "Bomb3 stop" << std::endl;
    }
}

// 计时状态下处理计时器滴答
void TimingState::OnTick(Bomb3 *bomb, uint8_t fineTime)
{
    if (bomb->timeout_ == 0) {
        std::cout << "OnTick error" << std::endl;
        return;
    }

    if (fineTime == 0) {
        bomb->timeout_--;
        PrintTimeout("remain", bomb->timeout_);
    }

    if (bomb->timeout_ == 0) {
        std::cout << "Bomb3 bomb!!! Reset for again test!" << std::endl;
        bomb->Tran(&Bomb3::setting_);
        bomb->timeout_ = TIMEOUT_INITIAL;
    }
}

// 主函数
int main()
{
    // 初始化队列
    QueueCtor(&keyQueue, keyBuffer, 10);
    Bomb3 bomp3;
    bomp3.Init(0xD);  // 初始化炸弹，密码为0xD

    // 创建运行线程
    std::thread t(Bomb3::Run, std::ref(bomp3));

    bool bombRunning = true;
    // 主循环处理键盘输入
    while (bombRunning) {
        switch (getch())
        {
        case 'u':
            QueueEnqueue(&keyQueue, (void *)SubState::SUB_STATE_UP);
            break;
        case 'd':
            QueueEnqueue(&keyQueue, (void *)SubState::SUB_STATE_DOWN);
            break;
        case 'a':
            QueueEnqueue(&keyQueue, (void *)SubState::SUB_STATE_ARM);
            break;
        case '\33':  // ESC键
            bombRunning = false;
            QueueEnqueue(&keyQueue, (void *)STATE_EXIT);
            break;
        default:
            break;
        }
    }
    
    // 等待线程结束
    t.join();
    std::cout << "main exit" << std::endl;

    return 0;
}