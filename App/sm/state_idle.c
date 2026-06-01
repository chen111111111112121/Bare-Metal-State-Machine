// App/sm/state_idle.c
#include "sm.h"
#include "log.h"

void idle_init(void) {
    LOG_INFO("enter IDLE");
    // 初始化代码
}

void idle_tick(void) {
    // 周期处理（可选）
}

void idle_event(event_t evt) {
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
