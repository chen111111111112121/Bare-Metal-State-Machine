// Bsp/soft_timer.c
#include "soft_timer.h"

typedef struct {
    timer_mode_t mode;
    uint32_t     period_ms;
    uint32_t     remain_ms;
    timer_cb_t   cb;
    uint8_t      active;
} timer_t;

static timer_t timers[SOFT_TIMER_MAX];
static uint32_t timer_count = 0;

void soft_timer_init(void) {
    for (int i = 0; i < SOFT_TIMER_MAX; i++) {
        timers[i].active = 0;
    }
    timer_count = 0;
}

uint32_t soft_timer_create(timer_mode_t mode, uint32_t ms, timer_cb_t cb) {
    if (timer_count >= SOFT_TIMER_MAX) {
        return -1;
    }
    uint32_t id = timer_count++;
    timers[id].mode = mode;
    timers[id].period_ms = ms;
    timers[id].remain_ms = ms;
    timers[id].cb = cb;
    timers[id].active = 1;
    return id;
}

void soft_timer_start(uint32_t id) {
    if (id < SOFT_TIMER_MAX) {
        timers[id].remain_ms = timers[id].period_ms;
        timers[id].active = 1;
    }
}

void soft_timer_stop(uint32_t id) {
    if (id < SOFT_TIMER_MAX) {
        timers[id].active = 0;
    }
}

void soft_timer_update(void) {
    for (int i = 0; i < timer_count; i++) {
        if (!timers[i].active) continue;

        if (timers[i].remain_ms > 0) {
            timers[i].remain_ms--;
        }

        if (timers[i].remain_ms == 0) {
            if (timers[i].cb) {
                timers[i].cb();
            }
            if (timers[i].mode == TIMER_REPEAT) {
                timers[i].remain_ms = timers[i].period_ms;
            } else {
                timers[i].active = 0;
            }
        }
    }
}
