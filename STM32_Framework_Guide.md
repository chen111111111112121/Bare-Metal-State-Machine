# STM32F103 裸机状态机框架搭建指南

> 本文档供 Claude Code CLI 读取，用于从 CubeMX 生成的 STM32F103 工程搭建最小可用的事件驱动状态机框架。
> 调试方式：SEGGER RTT（库文件由用户自行移植）。

---

## 1. CubeMX 配置要求（用户手动完成）

### 1.1 基础时钟

- HSE → PLL → SYSCLK 72MHz
- SysTick 1ms（CubeMX 默认，不需要改）

### 1.2 需要的外设

| 外设 | 配置 | 说明 |
|------|------|------|
| GPIO_EXTI | 1 个按键引脚，下降沿中断 | 示例：PA0 |
| IWDG | 可选，4 秒超时 | 看门狗 |

其他外设（SPI、I2C、ADC、UART 等）等需要时再加。
调试使用 RTT，不需要配置 UART。

### 1.3 NVIC 中断

| 中断 | 优先级 |
|------|--------|
| EXTI0（按键） | 1 |
| SysTick | 默认 |

### 1.4 生成设置

- Toolchain 选你用的（STM32CubeIDE / Makefile / Keil）
- 勾选 "Generate peripheral initialization as a pair of .c/.h files"

---

## 2. 目录结构

CubeMX 生成后，在工程根目录创建：

```
project_root/
├── Core/                       ← CubeMX 生成，不动
│   ├── Inc/
│   └── Src/
│
├── Bsp/                        ← 板级驱动层
│   ├── event_queue.h
│   ├── event_queue.c
│   ├── soft_timer.h
│   ├── soft_timer.c
│   ├── key.h
│   ├── key.c
│   ├── log.h
│   └── log.c
│
└── App/                        ← 应用层
    └── sm/
        ├── sm.h
        └── sm.c
```

---

## 3. 模块规格

### 3.1 事件队列

环形缓冲区，中断中写，主循环中读。

```c
// event_queue.h
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>

typedef enum {
    EVT_NONE = 0,
    EVT_KEY_SHORT,      // 短按
    EVT_KEY_LONG,       // 长按（1 秒）
    EVT_TIMER_100MS,    // 100ms 周期事件
    EVT_TIMER_1S,       // 1s 周期事件
    // 按需扩展...
} event_id_t;

typedef struct {
    event_id_t id;
    uint32_t   param;   // 附加参数
} event_t;

void evt_queue_init(void);
int  evt_queue_post(event_id_t id, uint32_t param);  // 中断中调用
int  evt_queue_get(event_t *out);                      // 主循环中调用

#endif
```

实现要点：
- 队列大小 16
- `post` 队列满时返回 -1，不阻塞
- `get` 队列空时返回 -1

### 3.2 软件定时器

一个 SysTick 驱动多个软件定时器，不需要额外硬件定时器。

```c
// soft_timer.h
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
void     soft_timer_update(void);   // 主循环中调用

#endif
```

nRF 迁移对照：
```c
// nRF:  app_timer_create(&id, APP_TIMER_MODE_REPEATED, cb);
// 这里: id = soft_timer_create(TIMER_REPEAT, 1000, cb);
```

### 3.3 状态机框架

最小状态机：3 个状态 + 状态表驱动。

```c
// sm.h
#ifndef SM_H
#define SM_H

#include "event_queue.h"

typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_COUNT        // 状态总数，不要改这个枚举
} sm_state_t;

void sm_init(void);
void sm_event(event_t evt);    // 投递事件
void sm_tick(void);            // 主循环调用
sm_state_t sm_get_state(void);
void sm_jump(sm_state_t new_state);

#endif
```

状态机实现模式（`sm.c` 中）：

```c
// 每个状态用一个函数指针结构体描述
typedef struct {
    void (*init)(void);
    void (*tick)(void);
    void (*event)(event_t evt);
} sm_handler_t;

// 状态表
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]  = { idle_init,  idle_tick,  idle_event  },
    [SM_WORK]  = { work_init,  work_tick,  work_event  },
    [SM_SLEEP] = { sleep_init, sleep_tick, sleep_event },
};

static sm_state_t current_state = SM_IDLE;

void sm_jump(sm_state_t new_state) {
    LOG_INFO("sm: %d -> %d", current_state, new_state);
    current_state = new_state;
    if (sm_table[current_state].init) {
        sm_table[current_state].init();
    }
}

void sm_tick(void) {
    if (sm_table[current_state].tick) {
        sm_table[current_state].tick();
    }
}

void sm_event(event_t evt) {
    if (sm_table[current_state].event) {
        sm_table[current_state].event(evt);
    }
}
```

每个状态的处理函数单独写（可以放在 `sm.c` 里，也可以拆成单独文件）：

```c
static void idle_init(void) { /* 进入 IDLE 时做初始化 */ }
static void idle_tick(void) { /* 周期性检查 */ }
static void idle_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_WORK);   // 按键切换到 WORK
            break;
        default:
            break;
    }
}
```

### 3.4 按键驱动

```c
// key.h
#ifndef KEY_H
#define KEY_H
#include "event_queue.h"
void key_init(void);              // 配置 GPIO + EXTI
void key_poll(void);              // 主循环中检测长按
void key_exti_handler(void);      // EXTI 中断回调中调用
#endif
```

实现要点：
- EXTI 中断回调中做 50ms 消抖，消抖通过后调用 `evt_queue_post(EVT_KEY_SHORT, 0)`
- `key_poll()` 中检测长按 1 秒，超时后投递 `EVT_KEY_LONG`
- 中断回调函数 `HAL_GPIO_EXTI_Callback()` 放在 CubeMX 生成的中断文件中

### 3.5 调试日志（RTT）

```c
// log.h
#ifndef LOG_H
#define LOG_H

#include "SEGGER_RTT.h"

#define LOG_INFO(fmt, ...) SEGGER_RTT_printf(0, "[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  SEGGER_RTT_printf(0, "[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  SEGGER_RTT_printf(0, "[D] " fmt "\n", ##__VA_ARGS__)

#endif
```

- RTT 库由用户自行移植到工程中
- Channel 0 默认使用，可在 `SEGGER_RTT.h` 或 `SEGGER_RTT_Conf.h` 中配置缓冲区大小
- `log.c` 可以为空文件（纯宏实现，不需要额外代码）

---

## 4. main.c 改造指引

在 CubeMX 生成的 `main.c` 中修改：

### 添加头文件（USER CODE Includes 区域）
```c
#include "event_queue.h"
#include "soft_timer.h"
#include "key.h"
#include "sm.h"
#include "log.h"
```

### main() 函数
```c
int main(void) {
    /* CubeMX 生成的初始化保持不变 */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    /* 框架初始化 */
    soft_timer_init();
    evt_queue_init();
    key_init();

    LOG_INFO("system boot");

    /* 创建周期定时器 */
    soft_timer_create(TIMER_REPEAT, 100, on_timer_100ms);
    soft_timer_create(TIMER_REPEAT, 1000, on_timer_1s);

    /* 状态机初始化 */
    sm_init();

    /* 主循环 */
    for (;;) {
        soft_timer_update();
        key_poll();

        event_t evt;
        if (evt_queue_get(&evt) == 0) {
            sm_event(evt);
        }

        sm_tick();
    }
}
```

### 定时器回调
```c
static void on_timer_100ms(void) {
    evt_queue_post(EVT_TIMER_100MS, 0);
}

static void on_timer_1s(void) {
    evt_queue_post(EVT_TIMER_1S, 0);
}
```

### 中断回调
在 `stm32f1xx_it.c` 或单独文件中：
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        key_exti_handler();
    }
}
```

---

## 5. Claude Code 执行步骤

1. 读取 CubeMX 生成的 `Core/Inc/main.h` 确认引脚定义和句柄
2. 创建 `Bsp/event_queue.c/h`
3. 创建 `Bsp/soft_timer.c/h`
4. 创建 `Bsp/key.c/h`
5. 创建 `Bsp/log.c/h`（RTT 宏定义）
6. 创建 `App/sm/sm.h`（事件枚举、状态枚举、接口声明）
7. 创建 `App/sm/sm.c`（状态表、sm_jump、sm_event、sm_tick）
8. 修改 `Core/Src/main.c`（添加头文件、框架初始化、主循环）
9. 添加 `HAL_GPIO_EXTI_Callback` 按键中断回调
10. 编译验证

---

## 6. nRF → STM32 API 快速替换

| nRF | STM32 HAL |
|-----|-----------|
| `nrf_gpio_pin_set(pin)` | `HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET)` |
| `nrf_gpio_pin_clear(pin)` | `HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET)` |
| `nrf_gpio_pin_read(pin)` | `HAL_GPIO_ReadPin(GPIOA, pin)` |
| `nrf_delay_ms(ms)` | `HAL_Delay(ms)` |
| `NRF_LOG_INFO(...)` | `LOG_INFO(...)` (RTT) |
| `app_timer_create/start/stop` | `soft_timer_create/start/stop` |
| `APP_TIMER_TICKS(ms)` | 直接传 ms 数值 |
