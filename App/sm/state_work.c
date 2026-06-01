// App/sm/state_work.c
#include "sm.h"
#include "log.h"

void work_init(void) {
    LOG_INFO("enter WORK");
    // 启动传感器采集、启动上报定时器等
}

void work_tick(void) {
    // 持续监控（可选）
}

void work_event(event_t evt) {
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
