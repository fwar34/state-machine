#ifndef STATETBL_H
#define STATETBL_H

#include <stdint.h>

typedef struct EventTag {
    uint16_t signal;
} Event;

struct StateTableTag; // Forward declaration
typedef void (*Tran)(struct StateTableTag *me, const Event *e);
typedef void (*Initial)(struct StateTableTag *me);

typedef struct StateTableTag {
    uint8_t curState;
    Tran *stateTable;
    uint8_t stateNum;
    uint8_t signalNum;
    Initial initial;
} StateTable;

void StateTableCtor(StateTable *me, Tran *stateTable, uint8_t stateNum, uint8_t signalNum, Initial initState);
void StateTableInit(StateTable *me);
void StateTableDispatch(StateTable *me, const Event *e);
void StateTableEmpty(StateTable *me, const Event *e);

#define UNUSE(arg) (void)(arg)
#define TRAN(target) (((StateTable *)me)->curState = (target))

#endif // !STATETBL_H
