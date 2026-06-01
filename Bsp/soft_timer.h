// Bsp/soft_timer.h
#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include <stdint.h>

#define SOFT_TIMER_MAX 8

typedef enum {
    TIMER_ONCE,
    TIMER_REPEAT
} timer_mode_t;

typedef void (*timer_cb_t)(void);

void     soft_timer_init(void);
uint32_t soft_timer_create(timer_mode_t mode, uint32_t ms, timer_cb_t cb);
void     soft_timer_start(uint32_t id);
void     soft_timer_stop(uint32_t id);
void     soft_timer_update(void);

#endif
