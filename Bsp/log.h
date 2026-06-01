#ifndef LOG_H
#define LOG_H

#include "SEGGER_RTT.h"

#define LOG_INFO(fmt, ...) SEGGER_RTT_printf(0, "[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  SEGGER_RTT_printf(0, "[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  SEGGER_RTT_printf(0, "[D] " fmt "\n", ##__VA_ARGS__)

#endif
