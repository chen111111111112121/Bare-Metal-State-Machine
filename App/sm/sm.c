// App/sm/sm.c
#include "sm.h"
#include "log.h"

// ========== 状态处理函数结构体 ==========
typedef struct {
    void (*init)(void);
    void (*tick)(void);
    void (*event)(event_t evt);
} sm_handler_t;

// ========== 状态表 ==========
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]  = { idle_init,  idle_tick,  idle_event  },
    [SM_WORK]  = { work_init,  work_tick,  work_event  },
    [SM_SLEEP] = { sleep_init, sleep_tick, sleep_event },
};

// ========== 当前状态 ==========
static sm_state_t current_state = SM_IDLE;

// ========== 状态机接口实现 ==========

void sm_init(void) {
    current_state = SM_IDLE;
    if (sm_table[current_state].init) {
        sm_table[current_state].init();
    }
}

void sm_jump(sm_state_t new_state) {
    LOG_INFO("sm: %d -> %d", current_state, new_state);
    current_state = new_state;
    if (sm_table[current_state].init) {
        sm_table[current_state].init();
    }
}

void sm_event(event_t evt) {
    if (sm_table[current_state].event) {
        sm_table[current_state].event(evt);
    }
}

void sm_tick(void) {
    if (sm_table[current_state].tick) {
        sm_table[current_state].tick();
    }
}

sm_state_t sm_get_state(void) {
    return current_state;
}
