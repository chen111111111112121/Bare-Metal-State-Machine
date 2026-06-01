// App/sm/sm.c
#include "sm.h"
#include "log.h"

typedef struct {
    void (*init)(void);
    void (*tick)(void);
    void (*event)(event_t evt);
} sm_handler_t;

// 前向声明
static void idle_init(void);
static void idle_tick(void);
static void idle_event(event_t evt);

static void work_init(void);
static void work_tick(void);
static void work_event(event_t evt);

static void sleep_init(void);
static void sleep_tick(void);
static void sleep_event(event_t evt);

// 状态表
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]  = { idle_init,  idle_tick,  idle_event  },
    [SM_WORK]  = { work_init,  work_tick,  work_event  },
    [SM_SLEEP] = { sleep_init, sleep_tick, sleep_event },
};

static sm_state_t current_state = SM_IDLE;

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

void sm_tick(void) {
    if (sm_table[current_state].tick) {
        sm_table[current_state].tick();
    }
}

void sm_event(event_t evt) {
    if (sm_table[current_state].event) {
        sm_table[current_state].event(evt);
    }
}

sm_state_t sm_get_state(void) {
    return current_state;
}

// 状态处理函数 - 空实现，后续填充
static void idle_init(void) {
    LOG_INFO("enter IDLE");
}

static void idle_tick(void) {
    // 预留
}

static void idle_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_WORK);
            break;
        case EVT_TIMER_1S:
            sm_jump(SM_SLEEP);
            break;
        default:
            break;
    }
}

static void work_init(void) {
    LOG_INFO("enter WORK");
}

static void work_tick(void) {
    // 预留
}

static void work_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_SLEEP);
            break;
        case EVT_KEY_LONG:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}

static void sleep_init(void) {
    LOG_INFO("enter SLEEP");
}

static void sleep_tick(void) {
    // 预留
}

static void sleep_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}
