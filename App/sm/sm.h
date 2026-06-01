// App/sm/sm.h
#ifndef SM_H
#define SM_H

#include "event_queue.h"

// ========== 状态枚举 ==========
typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_COUNT
} sm_state_t;

// ========== 状态机接口 ==========
void sm_init(void);
void sm_event(event_t evt);
void sm_tick(void);
sm_state_t sm_get_state(void);
void sm_jump(sm_state_t new_state);

// ========== IDLE 状态函数 ==========
void idle_init(void);
void idle_tick(void);
void idle_event(event_t evt);

// ========== WORK 状态函数 ==========
void work_init(void);
void work_tick(void);
void work_event(event_t evt);

// ========== SLEEP 状态函数 ==========
void sleep_init(void);
void sleep_tick(void);
void sleep_event(event_t evt);

#endif
