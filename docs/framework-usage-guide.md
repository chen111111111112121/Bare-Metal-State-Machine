# STM32 状态机框架开发者指南

> 本文档面向开发者，说明如何使用本框架、添加新功能、扩展系统。

---

## 1. 架构概览

### 1.1 设计思路

```
┌─────────────────────────────────────────────────────────┐
│                      中断层                              │
│   SysTick / EXTI / UART / SPI ...                      │
│                     ↓ post                              │
├─────────────────────────────────────────────────────────┤
│                 事件队列 (event_queue)                   │
│            中断安全的环形缓冲区，大小16                   │
│                     ↓ get                               │
├─────────────────────────────────────────────────────────┤
│                   主循环层                               │
│   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│   │ 软件定时器    │  │ 按键驱动     │  │ 状态机       │ │
│   │ soft_timer   │  │ key          │  │ sm           │ │
│   └──────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 1.2 目录结构

```
project_root/
├── Core/                       ← CubeMX 生成（不要手动修改）
│   ├── Inc/
│   │   └── main.h
│   └── Src/
│       ├── main.c              ← 已集成框架（在 USER CODE 区域修改）
│       └── stm32f1xx_it.c      ← 中断回调（在 USER CODE 区域修改）
│
├── Bsp/                        ← 板级驱动层（硬件相关）
│   ├── event_queue.h/c         ← 事件队列
│   ├── soft_timer.h/c          ← 软件定时器
│   ├── key.h/c                 ← 按键驱动
│   └── log.h/c                 ← RTT 日志宏
│
├── App/                        ← 应用层（业务逻辑）
│   └── sm/
│       ├── sm.h                ← 状态机接口
│       └── sm.c                ← 状态机实现
│
└── RTT/                        ← SEGGER RTT 库（已移植）
```

---

## 2. 核心模块使用指南

### 2.1 事件队列 (event_queue)

**用途：** 中断与主循环之间的通信桥梁

**API：**
```c
// 初始化（main.c 中已调用）
void evt_queue_init(void);

// 投递事件（中断或主循环中调用）
// 返回 0 成功，-1 队列满
int evt_queue_post(event_id_t id, uint32_t param);

// 获取事件（主循环中调用）
// 返回 0 有事件，-1 队列空
int evt_queue_get(event_t *out);
```

**使用示例：**
```c
// 在定时器回调中投递事件
static void on_timer_100ms(void) {
    evt_queue_post(EVT_TIMER_100MS, 0);
}

// 在中断回调中投递事件
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    evt_queue_post(EVT_UART_RX, received_data);
}
```

### 2.2 软件定时器 (soft_timer)

**用途：** 周期性任务，不占用硬件定时器

**API：**
```c
// 初始化（main.c 中已调用）
void soft_timer_init(void);

// 创建定时器，返回定时器ID
// mode: TIMER_ONCE（单次）或 TIMER_REPEAT（重复）
// ms: 周期（毫秒）
// cb: 超时回调函数
uint32_t soft_timer_create(timer_mode_t mode, uint32_t ms, timer_cb_t cb);

// 启动/停止定时器
void soft_timer_start(uint32_t id);
void soft_timer_stop(uint32_t id);

// 更新定时器（主循环中已调用）
void soft_timer_update(void);
```

**使用示例：**
```c
// 创建 500ms 重复定时器
uint32_t blink_timer = soft_timer_create(TIMER_REPEAT, 500, on_blink);

// 创建 2s 单次定时器
uint32_t timeout_timer = soft_timer_create(TIMER_ONCE, 2000, on_timeout);

// 回调函数
static void on_blink(void) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    evt_queue_post(EVT_LED_TOGGLE, 0);
}
```

### 2.3 按键驱动 (key)

**用途：** 按键消抖、长短按检测

**API：**
```c
// 初始化（main.c 中已调用）
void key_init(void);

// 轮询检测长按（主循环中已调用）
void key_poll(void);

// EXTI 中断回调（stm32f1xx_it.c 中已调用）
void key_exti_handler(void);
```

**当前状态：** 桩实现（空函数），需要后续填充消抖逻辑。

### 2.4 状态机 (sm)

**用途：** 管理系统状态和状态转换

**API：**
```c
// 初始化（main.c 中已调用）
void sm_init(void);

// 投递事件给状态机（主循环中已调用）
void sm_event(event_t evt);

// 状态机周期处理（主循环中已调用）
void sm_tick(void);

// 获取当前状态
sm_state_t sm_get_state(void);

// 切换状态（会自动调用新状态的 init 函数）
void sm_jump(sm_state_t new_state);
```

### 2.5 日志 (log)

**用途：** 通过 RTT 输出调试信息

**宏定义：**
```c
LOG_INFO(fmt, ...)  // [I] 信息日志
LOG_ERR(fmt, ...)   // [E] 错误日志
LOG_DBG(fmt, ...)   // [D] 调试日志
```

**使用示例：**
```c
LOG_INFO("system boot");
LOG_ERR("sensor read failed: %d", error_code);
LOG_DBG("state: %d", sm_get_state());
```

---

## 3. 扩展指南

### 3.1 添加新状态

**步骤：**

1. **在 `App/sm/sm.h` 中添加状态枚举**
```c
typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_CONFIG,      // ← 新增
    SM_COUNT
} sm_state_t;
```

2. **在 `App/sm/sm.c` 中添加前向声明**
```c
static void config_init(void);
static void config_tick(void);
static void config_event(event_t evt);
```

3. **在状态表中注册**
```c
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]   = { idle_init,   idle_tick,   idle_event   },
    [SM_WORK]   = { work_init,   work_tick,   work_event   },
    [SM_SLEEP]  = { sleep_init,  sleep_tick,  sleep_event  },
    [SM_CONFIG] = { config_init, config_tick, config_event },  // ← 新增
};
```

4. **实现状态处理函数**
```c
static void config_init(void) {
    LOG_INFO("enter CONFIG");
    // 进入状态时的初始化
}

static void config_tick(void) {
    // 周期性处理（可选）
}

static void config_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_IDLE);  // 按键返回 IDLE
            break;
        case EVT_UART_RX:
            // 处理串口数据
            break;
        default:
            break;
    }
}
```

### 3.2 添加新事件

**步骤：**

1. **在 `Bsp/event_queue.h` 中添加事件枚举**
```c
typedef enum {
    EVT_NONE = 0,
    EVT_KEY_SHORT,
    EVT_KEY_LONG,
    EVT_TIMER_100MS,
    EVT_TIMER_1S,
    EVT_UART_RX,        // ← 新增
    EVT_SENSOR_DATA,    // ← 新增
} event_id_t;
```

2. **在需要的地方投递事件**
```c
// 定时器回调中
static void on_sensor_read(void) {
    int16_t temp = read_temperature();
    evt_queue_post(EVT_SENSOR_DATA, (uint32_t)temp);
}

// 中断回调中
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    evt_queue_post(EVT_UART_RX, rx_buffer[0]);
}
```

3. **在状态机中响应事件**
```c
static void work_event(event_t evt) {
    switch (evt.id) {
        case EVT_SENSOR_DATA:
            LOG_INFO("temp: %d", (int16_t)evt.param);
            break;
        case EVT_UART_RX:
            LOG_INFO("rx: %c", (char)evt.param);
            break;
        default:
            break;
    }
}
```

### 3.3 添加新定时器

**步骤：**

1. **定义回调函数**
```c
static void on_500ms(void) {
    evt_queue_post(EVT_TIMER_500MS, 0);
}
```

2. **创建定时器（在 main.c 的 USER CODE BEGIN 2 区域）**
```c
/* 创建周期定时器 */
soft_timer_create(TIMER_REPEAT, 100, on_timer_100ms);
soft_timer_create(TIMER_REPEAT, 1000, on_timer_1s);
soft_timer_create(TIMER_REPEAT, 500, on_500ms);  // ← 新增
```

3. **添加对应事件枚举（可选）**
```c
// 在 event_queue.h 中
EVT_TIMER_500MS,
```

### 3.4 添加新硬件模块

**示例：添加 DHT11 温湿度传感器**

1. **在 `Bsp/` 中创建模块文件**

```c
// Bsp/dht11.h
#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

typedef struct {
    int16_t temperature;
    int16_t humidity;
} dht11_data_t;

void dht11_init(void);
int  dht11_read(dht11_data_t *data);

#endif
```

```c
// Bsp/dht11.c
#include "dht11.h"
#include "main.h"

// 实现 GPIO 时序读取 DHT11
// ...
```

2. **在 `CMakeLists.txt` 中添加源文件**
```cmake
# Bsp sources
${CMAKE_SOURCE_DIR}/Bsp/dht11.c  # ← 新增
```

3. **在 main.c 中初始化和使用**
```c
#include "dht11.h"

int main(void) {
    // ... 其他初始化 ...
    dht11_init();
    
    // 创建读取定时器
    soft_timer_create(TIMER_REPEAT, 2000, on_dht11_read);
}

static void on_dht11_read(void) {
    dht11_data_t data;
    if (dht11_read(&data) == 0) {
        evt_queue_post(EVT_SENSOR_TEMP, data.temperature);
        evt_queue_post(EVT_SENSOR_HUMI, data.humidity);
    }
}
```

---

## 4. 常见场景示例

### 4.1 LED 闪烁控制

```c
// 定义 LED 状态
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,
    LED_BLINK_FAST,
} led_mode_t;

static led_mode_t led_mode = LED_OFF;
static uint32_t blink_timer;

static void work_init(void) {
    led_mode = LED_BLINK_FAST;
    blink_timer = soft_timer_create(TIMER_REPEAT, 200, on_blink);
}

static void on_blink(void) {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}
```

### 4.2 按键切换状态

```c
static void idle_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_WORK);
            break;
        case EVT_KEY_LONG:
            sm_jump(SM_CONFIG);
            break;
        default:
            break;
    }
}
```

### 4.3 串口命令解析

```c
static void work_event(event_t evt) {
    switch (evt.id) {
        case EVT_UART_RX:
            handle_uart_cmd((char)evt.param);
            break;
        default:
            break;
    }
}

static void handle_uart_cmd(char cmd) {
    switch (cmd) {
        case '1':
            sm_jump(SM_MODE1);
            break;
        case '2':
            sm_jump(SM_MODE2);
            break;
        case 's':
            LOG_INFO("current state: %d", sm_get_state());
            break;
        default:
            LOG_ERR("unknown cmd: %c", cmd);
            break;
    }
}
```

### 4.4 超时处理

```c
static uint32_t timeout_timer;

static void work_init(void) {
    // 5秒无操作则返回 IDLE
    timeout_timer = soft_timer_create(TIMER_ONCE, 5000, on_timeout);
}

static void work_event(event_t evt) {
    // 重置超时定时器
    soft_timer_start(timeout_timer);
    
    switch (evt.id) {
        case EVT_KEY_SHORT:
            // 处理按键
            break;
        default:
            break;
    }
}

static void on_timeout(void) {
    LOG_INFO("timeout, back to IDLE");
    sm_jump(SM_IDLE);
}
```

---

## 5. 主循环流程说明

```c
for (;;) {
    // 1. 更新软件定时器（每 1ms 调用一次）
    soft_timer_update();
    
    // 2. 轮询按键（检测长按）
    key_poll();
    
    // 3. 获取事件
    event_t evt;
    if (evt_queue_get(&evt) == 0) {
        // 4. 投递事件给状态机
        sm_event(evt);
    }
    
    // 5. 状态机周期处理
    sm_tick();
    
    // 6. 延时 1ms（控制循环频率）
    HAL_Delay(1);
}
```

**注意：**
- 定时器回调在 `soft_timer_update()` 中执行（主循环上下文，非中断）
- 事件队列是中断与主循环的桥梁
- 状态机处理在主循环中，可以安全调用 HAL 库函数

---

## 6. 调试技巧

### 6.1 使用 RTT 日志

```c
// 打印状态切换
LOG_INFO("state: %d -> %d", old_state, new_state);

// 打印事件
LOG_DBG("event: id=%d param=%lu", evt.id, evt.param);

// 打印错误
LOG_ERR("queue full, event lost: %d", evt_id);
```

### 6.2 查看当前状态

```c
sm_state_t state = sm_get_state();
LOG_INFO("current state: %d", state);
```

### 6.3 监控定时器

```c
// 在定时器回调中打印
static void on_timer_1s(void) {
    LOG_DBG("1s timer fired");
    evt_queue_post(EVT_TIMER_1S, 0);
}
```

---

## 7. 注意事项

### 7.1 CubeMX 代码保护

- **所有自定义代码必须放在 `USER CODE BEGIN/END` 区域内**
- CubeMX 重新生成代码时会保留这些区域
- 不要修改 CubeMX 生成的其他代码

### 7.2 中断安全

- `evt_queue_post()` 可以在中断中调用（已使用 volatile）
- `evt_queue_get()` 只在主循环中调用
- 定时器回调在主循环中执行，可以调用 HAL 库

### 7.3 事件队列容量

- 队列大小 16，最多缓存 15 个事件
- 如果事件产生过快，可能丢失事件
- 解决方案：增加队列大小或降低事件频率

### 7.4 状态机设计原则

- 每个状态只处理自己关心的事件
- 使用 `default` 分支忽略未知事件
- 状态切换时清理资源（在旧状态的 exit 或新状态的 init 中）

---

## 8. 快速参考

### 事件投递位置

| 位置 | 函数 | 示例 |
|------|------|------|
| 定时器回调 | `soft_timer_create()` 的 cb 参数 | `on_timer_100ms()` |
| EXTI 中断 | `HAL_GPIO_EXTI_Callback()` | `key_exti_handler()` |
| 串口中断 | `HAL_UART_RxCpltCallback()` | 投递 EVT_UART_RX |
| 主循环 | 直接调用 `evt_queue_post()` | 轮询检测 |

### 文件修改指南

| 需求 | 修改文件 |
|------|---------|
| 添加新状态 | `App/sm/sm.h`, `App/sm/sm.c` |
| 添加新事件 | `Bsp/event_queue.h` |
| 添加新硬件模块 | `Bsp/xxx.h/c`, `CMakeLists.txt` |
| 添加定时器 | `Core/Src/main.c` (USER CODE 区域) |
| 添加中断回调 | `Core/Src/stm32f1xx_it.c` (USER CODE 区域) |

---

## 9. 后续扩展建议

1. **实现按键驱动** - 添加消抖和长短按检测
2. **添加更多状态** - 根据业务需求设计状态机
3. **添加串口通信** - 实现命令解析和数据上报
4. **添加传感器** - 集成温度、湿度、光照等传感器
5. **添加显示模块** - OLED 或 LCD 显示
6. **添加低功耗模式** - 睡眠状态进入 STOP 模式

---

**框架版本：** v1.0  
**最后更新：** 2026-06-01  
**适用芯片：** STM32F103C8T6  
**开发环境：** CMake + GCC ARM
