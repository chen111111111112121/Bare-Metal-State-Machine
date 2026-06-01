// Bsp/event_queue.h
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>

typedef enum {
    EVT_NONE = 0,
    EVT_KEY_SHORT,
    EVT_KEY_LONG,
    EVT_TIMER_100MS,
    EVT_TIMER_1S,
} event_id_t;

typedef struct {
    event_id_t id;
    uint32_t   param;
} event_t;

void evt_queue_init(void);
int  evt_queue_post(event_id_t id, uint32_t param);
int  evt_queue_get(event_t *out);

#endif
