/**
 * @file RTT_DEBUG.h
 * @brief SEGGER RTT helpers: printf-style log, hex dump, AT return-code strings.
 */
#ifndef __RTT_DEBUG_H__
#define __RTT_DEBUG_H__

#include <stddef.h>
#include <stdint.h>

#include "SEGGER_RTT.h"

#ifdef __cplusplus
extern "C" {
#endif

void rtt_log(uint8_t terminal, const char *format, ...);

/** Call SEGGER_RTT_Init once (safe to call after HAL_Init). */
void RTT_Debug_Init(void);

/** Hex dump: 16 bytes per line + ASCII. terminal 0..15; tag may be NULL. */
void rtt_hex_dump(uint8_t terminal, const char *tag, const void *data, size_t len);

/** Print string; non-printable -> '.'; max_len caps scan length. */
void rtt_safe_str_dump(uint8_t terminal, const char *tag, const char *s, size_t max_len);

/** Labels for AT_RTOS_* numeric codes (see at_rtos_session.h). */
const char *rtt_at_rtos_rc_str(int rc);

/** Log one line: where, rc, and rtt_at_rtos_rc_str(rc). */
void rtt_at_rtos_log_rc(const char *where, int rc);

#define RTT_HEX_DUMP(tag, ptr, len) rtt_hex_dump(0, (tag), (ptr), (len))
#define RTT_STR_DUMP(tag, s, max_len) rtt_safe_str_dump(0, (tag), (s), (max_len))

#define RTT_LOG(fmt, ...) rtt_log(0, fmt, ##__VA_ARGS__)
#define RTT_LOG_TO(term, fmt, ...) rtt_log((term), fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
