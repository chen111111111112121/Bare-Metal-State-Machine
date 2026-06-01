// Bsp/key.h
#ifndef KEY_H
#define KEY_H

#include "event_queue.h"

void key_init(void);
void key_poll(void);
void key_exti_handler(void);

#endif
