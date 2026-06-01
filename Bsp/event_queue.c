// Bsp/event_queue.c
#include "event_queue.h"

#define EVT_QUEUE_SIZE 16

static event_t queue[EVT_QUEUE_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;

void evt_queue_init(void) {
    head = 0;
    tail = 0;
}

int evt_queue_post(event_id_t id, uint32_t param) {
    uint8_t next = (head + 1) % EVT_QUEUE_SIZE;
    if (next == tail) {
        return -1;  // 队列满
    }
    queue[head].id = id;
    queue[head].param = param;
    head = next;
    return 0;
}

int evt_queue_get(event_t *out) {
    if (head == tail) {
        return -1;  // 队列空
    }
    *out = queue[tail];
    tail = (tail + 1) % EVT_QUEUE_SIZE;
    return 0;
}
