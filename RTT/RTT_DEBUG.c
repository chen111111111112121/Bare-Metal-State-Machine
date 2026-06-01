/**
 ****************************************************************************************************
 * @file        rtt_log.c
 * @author      csd
 * @version     V1.0
 * @date        2025-09-28
 * @brief       RTT ????????? ????????
 * @license     Copyright (c) 2025
 ****************************************************************************************************
 * @attention
 *
 * ???????:
 * 1. ??? SEGGER RTT ?????????? printf ??????????
 * 2. ?? rtt_log() ????????????????? (0~15) ??????????
 * 3. ?? RTT_LOG ????????????? 0
 * 4. ?? RTT_LOG_TO ??????????????????
 * 5. ????????????????????????????? UART ???
 *
 ****************************************************************************************************
 */


#include "RTT_DEBUG.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint8_t s_rtt_inited;

void RTT_Debug_Init(void)
{
  if (s_rtt_inited != 0u)
  {
    return;
  }
  SEGGER_RTT_Init();
  s_rtt_inited = 1u;
  rtt_log(0, "[RTT] init ok\r\n");
}

void rtt_log(uint8_t terminal, const char *format, ...)
{
  va_list args;

  if (s_rtt_inited == 0u)
  {
    SEGGER_RTT_Init();
    s_rtt_inited = 1u;
  }

  SEGGER_RTT_SetTerminal(terminal);

  va_start(args, format);
  (void)SEGGER_RTT_vprintf(0, format, &args);
  va_end(args);
}

void rtt_hex_dump(uint8_t terminal, const char *tag, const void *data, size_t len)
{
  const uint8_t *p = (const uint8_t *)data;
  if (tag != NULL)
  {
    rtt_log(terminal, "[%s] hex %u bytes\r\n", tag, (unsigned)len);
  }
  else
  {
    rtt_log(terminal, "[hex] %u bytes\r\n", (unsigned)len);
  }

  for (size_t base = 0u; base < len; base += 16u)
  {
    char line[96];
    size_t n = len - base;
    if (n > 16u)
    {
      n = 16u;
    }
    int o = snprintf(line, sizeof(line), "%04u: ", (unsigned)base);
    if (o < 0)
    {
      o = 0;
    }
    for (size_t i = 0u; i < n; i++)
    {
      int w = snprintf(line + (size_t)o, sizeof(line) - (size_t)o, "%02X ", (unsigned)p[base + i]);
      if (w > 0)
      {
        o += w;
      }
    }
    for (size_t i = n; i < 16u; i++)
    {
      int w = snprintf(line + (size_t)o, sizeof(line) - (size_t)o, "   ");
      if (w > 0)
      {
        o += w;
      }
    }
    {
      int w = snprintf(line + (size_t)o, sizeof(line) - (size_t)o, "| ");
      if (w > 0)
      {
        o += w;
      }
    }
    for (size_t i = 0u; i < n; i++)
    {
      uint8_t c = p[base + i];
      char ch = (char)((c >= 32u && c <= 126u) ? c : '.');
      int w = snprintf(line + (size_t)o, sizeof(line) - (size_t)o, "%c", ch);
      if (w > 0)
      {
        o += w;
      }
    }
    (void)snprintf(line + (size_t)o, sizeof(line) - (size_t)o, "|\r\n");
    SEGGER_RTT_SetTerminal(terminal);
    (void)SEGGER_RTT_WriteString(0, line);
  }
}

void rtt_safe_str_dump(uint8_t terminal, const char *tag, const char *s, size_t max_len)
{
  if (s == NULL)
  {
    rtt_log(terminal, "[%s] (null)\r\n", (tag != NULL) ? tag : "str");
    return;
  }
  if (tag != NULL)
  {
    rtt_log(terminal, "[%s] str:\r\n", tag);
  }
  size_t chunk = 0u;
  for (size_t i = 0u; i < max_len && s[i] != '\0'; i++)
  {
    char c = s[i];
    if (c < 32 || c > 126)
    {
      c = '.';
    }
    SEGGER_RTT_SetTerminal(terminal);
    (void)SEGGER_RTT_PutChar(0, c);
    chunk++;
    if (chunk >= 128u)
    {
      (void)SEGGER_RTT_PutChar(0, '\n');
      chunk = 0u;
    }
  }
  (void)SEGGER_RTT_PutChar(0, '\r');
  (void)SEGGER_RTT_PutChar(0, '\n');
}

const char *rtt_at_rtos_rc_str(int rc)
{
  switch (rc)
  {
  case 0:
    return "OK";
  case -1:
    return "TIMEOUT";
  case -2:
    return "IO";
  case -3:
    return "ARG";
  case -4:
    return "PARSE";
  case -5:
    return "PROTO";
  case -6:
    return "BUSY";
  default:
    return "UNKNOWN";
  }
}

void rtt_at_rtos_log_rc(const char *where, int rc)
{
  rtt_log(0, "[AT] %s -> %d %s\r\n", (where != NULL) ? where : "?", rc, rtt_at_rtos_rc_str(rc));
}

