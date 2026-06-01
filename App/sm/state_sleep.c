// App/sm/state_sleep.c
#include "sm.h"
#include "log.h"

void sleep_init(void) {
    LOG_INFO("enter SLEEP");
    // 停止采集、低功耗配置等
}

void sleep_tick(void) {
    // 周期处理（可选）
}

void sleep_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}
