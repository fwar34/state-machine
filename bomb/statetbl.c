#include "statetbl.h"

void StateTableCtor(StateTable *me, Tran *stateTable, uint8_t stateNum, uint8_t signalNum, Initial Initial)
{
    me->stateTable = stateTable;
    me->stateNum = stateNum;
    me->signalNum = signalNum;
    me->initial = Initial;
}

void StateTableInit(StateTable *me)
{
    me->curState = me->stateNum;
    (me->initial)(me);
}

void StateTableDispatch(StateTable *me, const Event *e)
{
    if (e->signal >= me->signalNum) {
        return;
    }

    me->stateTable[me->curState * me->signalNum + e->signal](me, e);

    if (me->curState >= me->stateNum) {
        // error
    }
}

void StateTableEmpty(StateTable *me, const Event *e)
{
    UNUSE(me);
    UNUSE(e);
}
