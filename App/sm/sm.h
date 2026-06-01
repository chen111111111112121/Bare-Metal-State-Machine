// App/sm/sm.h
#ifndef SM_H
#define SM_H

#include "event_queue.h"

typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_COUNT
} sm_state_t;

void sm_init(void);
void sm_event(event_t evt);
void sm_tick(void);
sm_state_t sm_get_state(void);
void sm_jump(sm_state_t new_state);

#endif
