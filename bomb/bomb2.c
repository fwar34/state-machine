#include "statetbl.h"
#include "sync_queue.h"
#include <stdio.h>
#include <unistd.h>
#include <conio.h>
#include <pthread.h>

#define BOMB2_INIT_TIMEOUT 15
#define BOMB2_MIN_TIMEOUT 10
#define BOMB2_MAX_TIMEOUT 120
#define TICK_INTERVAL_100MS 100

typedef enum {
    BOMB_STATE_SETTING,
    BOMB_STATE_TIMING,
    BOMB_STATE_MAX,
} BombState;

typedef enum {
    BOMB_SIGNAL_UP,
    BOMB_SIGNAL_DOWN,
    BOMB_SIGNAL_ARM,
    BOMB_SIGNAL_TICK,
    BOMB_SIGNAL_MAX,
} BombSignal;

#define STATE_NUM 2
#define SIGNAL_NUM 4

typedef struct Bomb2Tag {
    StateTable super;
    uint32_t timeout;
    uint8_t passwd;
    uint8_t curInput;
} Bomb2;

static Bomb2 g_bomb2;

typedef struct TickEventTag {
    Event super;
    uint16_t fineTime;
} TickEvent;

static void DisplayTimeout(uint32_t timeout)
{
    printf("curTimeout[%d]\n", timeout);
}

static void DisplayCurInput(const char c, uint8_t curInput)
{
    printf("%c, curInput[0x%02x]\n", c, curInput);
}

void BombSettingUp(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    if (me->timeout < BOMB2_MAX_TIMEOUT) {
        me->timeout++;
    }
    DisplayTimeout(me->timeout);
}
void BombSettingDown(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    if (me->timeout > BOMB2_MIN_TIMEOUT) {
        me->timeout--;
    }
    DisplayTimeout(me->timeout);
}
void BombSettingArm(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    printf("Bomb2 start\n");
    DisplayTimeout(me->timeout);
    me->curInput = 0;
    TRAN(BOMB_STATE_TIMING);
}
void BombTimingUp(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    me->curInput <<= 1;
    me->curInput |= 1;
    DisplayCurInput('u', me->curInput);
}
void BombTimingDown(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    me->curInput <<= 1;
    DisplayCurInput('d', me->curInput);
}
void BombTimingArm(Bomb2 *me, const Event *e)
{
    UNUSE(e);
    printf("Bomb2 stop, curInput[0x%02x]\n", me->curInput);
    if (me->curInput == me->passwd) {
        me->curInput = 0;
        TRAN(BOMB_STATE_SETTING);
    }
    DisplayTimeout(me->timeout);
}
void BombTimingTick(Bomb2 *me, const TickEvent *e)
{
    if (me->timeout == 0) {
        // error
        printf("Timing: Tick, error fineTime:%d\n", e->fineTime);
        return;
    }

    if (e->fineTime == 0) {
        me->timeout--;
        DisplayTimeout(me->timeout);
    }
    if (me->timeout == 0) {
        printf("Bomb2 bomb!!! Reset for again test!\n");
        me->timeout = BOMB2_INIT_TIMEOUT;
        TRAN(BOMB_STATE_SETTING);
    }
}

static Tran stateTable[STATE_NUM][SIGNAL_NUM] = {
    {(Tran)BombSettingUp, (Tran)BombSettingDown, (Tran)BombSettingArm, StateTableEmpty},
    {(Tran)BombTimingUp, (Tran)BombTimingDown, (Tran)BombTimingArm, (Tran)BombTimingTick},
};

static SyncQueue keyQueue;
static void *keyBuffer[10];

static void Bomb2Initial(StateTable *me)
{
    Bomb2 *bomb2 = (Bomb2 *)me;
    bomb2->timeout = BOMB2_INIT_TIMEOUT;
    bomb2->passwd = 0xD;
    TRAN(BOMB_STATE_SETTING);
    printf("Bomb2Initial...\n");
}

void* Bomb2Run(void *arg)
{
    UNUSE(arg);
    StateTableInit((StateTable *)&g_bomb2);

    char key;
    for (;;) {
        bool isTimeout = false;
        key = (char)(uintptr_t)QueueDequeueWithTimeout(&keyQueue, TICK_INTERVAL_100MS, &isTimeout);
        if (isTimeout) {
            static TickEvent tickEvent = {{BOMB_SIGNAL_TICK}, 0};
            if (++tickEvent.fineTime == 10) {
                tickEvent.fineTime = 0;
            }
            StateTableDispatch((StateTable *)&g_bomb2, (Event *)&tickEvent);
        } else {
            static const Event upEvent = {BOMB_SIGNAL_UP};
            static const Event downEvent = {BOMB_SIGNAL_DOWN};
            static const Event armEvent = {BOMB_SIGNAL_ARM};
            static const Event *e = NULL;
        
            switch (key)
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
            case '\33':
                return NULL;
            default:
            }

            if (e != NULL) {
                StateTableDispatch((StateTable *)&g_bomb2, e);
                e = NULL;
            }
        }
    }

    return NULL;
}

int main()
{
    StateTableCtor((StateTable *)&g_bomb2, &stateTable[0][0], STATE_NUM, SIGNAL_NUM, Bomb2Initial);
    QueueCtor(&keyQueue, keyBuffer, 10);

    pthread_t bomb2Thread;
    pthread_create(&bomb2Thread, NULL, Bomb2Run, NULL);

    bool bombRunning = true;
    while (bombRunning) {
        switch (getch())
        {
        case 'u':
            QueueEnqueue(&keyQueue, (void *)'u');
            break;
        case 'd':
            QueueEnqueue(&keyQueue, (void *)'d');
            break;
        case 'a':
            QueueEnqueue(&keyQueue, (void *)'a');
            break;
        case '\33':
            bombRunning = false;
            QueueEnqueue(&keyQueue, (void *)'\33');
            break;
        default:
        }
    }
    pthread_join(bomb2Thread, NULL);
    printf("main exit\n");

    return 0;
}