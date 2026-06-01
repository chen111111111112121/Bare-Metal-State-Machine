# STM32 状态机框架实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 搭建通用裸机状态机框架，支持事件驱动、软件定时器、RTT调试输出

**Architecture:** 分层设计 - Bsp层（硬件相关，中断安全）通过事件队列与App层（业务逻辑，主循环执行）通信。状态机采用表驱动设计，便于扩展。

**Tech Stack:** STM32 HAL + SEGGER RTT + 环形缓冲区事件队列 + SysTick软件定时器

---

## 文件结构

### 新建文件（Bsp 层）

| 文件 | 职责 |
|------|------|
| `Bsp/event_queue.h/c` | 事件队列：中断→主循环通信 |
| `Bsp/soft_timer.h/c` | 软件定时器：SysTick驱动，不占用硬件定时器 |
| `Bsp/key.h/c` | 按键驱动：消抖、长按检测（桩实现） |
| `Bsp/log.h/c` | RTT日志宏：调试输出 |

### 新建文件（App 层）

| 文件 | 职责 |
|------|------|
| `App/sm/sm.h` | 状态机接口：状态枚举、事件枚举、接口声明 |
| `App/sm/sm.c` | 状态机实现：状态表、sm_jump、sm_event、sm_tick |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `Core/Src/main.c` | 添加框架初始化、主循环集成 |
| `Core/Src/stm32f1xx_it.c` | 添加 GPIO_EXTI 中断回调 |
| `CMakeLists.txt` | 添加新源文件路径 |

---

## 实施任务

### Task 1: 创建日志模块

**目标：** 配置 RTT 日志宏，方便后续调试

**Files:**
- Create: `Bsp/log.h`
- Create: `Bsp/log.c`

---

- [ ] **Step 1.1: 创建 log.h 头文件**

```c
// Bsp/log.h
#ifndef LOG_H
#define LOG_H

#include "SEGGER_RTT.h"

#define LOG_INFO(fmt, ...) SEGGER_RTT_printf(0, "[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  SEGGER_RTT_printf(0, "[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  SEGGER_RTT_printf(0, "[D] " fmt "\n", ##__VA_ARGS__)

#endif
```

---

- [ ] **Step 1.2: 创建 log.c 实现文件**

```c
// Bsp/log.c
// 纯宏实现，此文件可为空
// RTT库文件已移植在 RTT/ 目录
```

---

- [ ] **Step 1.3: 验证日志编译**

在 main.c 中临时添加测试代码：

```c
#include "log.h"

int main(void) {
    // ... 现有初始化 ...
    LOG_INFO("test log");
    while (1) {}
}
```

运行：`make` 或 `cmake --build build`
预期：编译通过，无错误

---

- [ ] **Step 1.4: 移除测试代码，恢复 main.c**

从 main.c 中移除 Step 1.3 添加的测试代码。

---

- [ ] **Step 1.5: Commit 日志模块**

```bash
git add Bsp/log.h Bsp/log.c
git commit -m "feat(bsp): add RTT log macros"
```

---

### Task 2: 创建事件队列模块

**目标：** 实现中断安全的事件队列

**Files:**
- Create: `Bsp/event_queue.h`
- Create: `Bsp/event_queue.c`

---

- [ ] **Step 2.1: 创建 event_queue.h 头文件**

```c
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
```

---

- [ ] **Step 2.2: 创建 event_queue.c 实现文件**

```c
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
```

---

- [ ] **Step 2.3: Commit 事件队列模块**

```bash
git add Bsp/event_queue.h Bsp/event_queue.c
git commit -m "feat(bsp): add event queue with circular buffer"
```

---

### Task 3: 创建软件定时器模块

**目标：** 实现 SysTick 驱动的软件定时器

**Files:**
- Create: `Bsp/soft_timer.h`
- Create: `Bsp/soft_timer.c`

---

- [ ] **Step 3.1: 创建 soft_timer.h 头文件**

```c
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
```

---

- [ ] **Step 3.2: 创建 soft_timer.c 实现文件**

```c
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
```

---

- [ ] **Step 3.3: Commit 软件定时器模块**

```bash
git add Bsp/soft_timer.h Bsp/soft_timer.c
git commit -m "feat(bsp): add software timer module"
```

---

### Task 4: 创建按键驱动模块（桩实现）

**目标：** 预留按键接口，后续扩展

**Files:**
- Create: `Bsp/key.h`
- Create: `Bsp/key.c`

---

- [ ] **Step 4.1: 创建 key.h 头文件**

```c
// Bsp/key.h
#ifndef KEY_H
#define KEY_H

#include "event_queue.h"

void key_init(void);
void key_poll(void);
void key_exti_handler(void);

#endif
```

---

- [ ] **Step 4.2: 创建 key.c 实现文件（桩）**

```c
// Bsp/key.c
#include "key.h"

void key_init(void) {
    // 预留：配置 GPIO 和 EXTI
}

void key_poll(void) {
    // 预留：检测长按
}

void key_exti_handler(void) {
    // 预留：EXTI 中断回调
}
```

---

- [ ] **Step 4.3: Commit 按键驱动模块**

```bash
git add Bsp/key.h Bsp/key.c
git commit -m "feat(bsp): add key driver stub"
```

---

### Task 5: 创建状态机模块

**目标：** 实现表驱动状态机

**Files:**
- Create: `App/sm/sm.h`
- Create: `App/sm/sm.c`

---

- [ ] **Step 5.1: 创建 sm.h 头文件**

```c
// App/sm/sm.h
#ifndef SM_H
#define SM_H

#include "event_queue.h"

typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_COUNT
} sm_state_t;

void sm_init(void);
void sm_event(event_t evt);
void sm_tick(void);
sm_state_t sm_get_state(void);
void sm_jump(sm_state_t new_state);

#endif
```

---

- [ ] **Step 5.2: 创建 sm.c 实现文件**

```c
// App/sm/sm.c
#include "sm.h"
#include "log.h"

typedef struct {
    void (*init)(void);
    void (*tick)(void);
    void (*event)(event_t evt);
} sm_handler_t;

// 前向声明
static void idle_init(void);
static void idle_tick(void);
static void idle_event(event_t evt);

static void work_init(void);
static void work_tick(void);
static void work_event(event_t evt);

static void sleep_init(void);
static void sleep_tick(void);
static void sleep_event(event_t evt);

// 状态表
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]  = { idle_init,  idle_tick,  idle_event  },
    [SM_WORK]  = { work_init,  work_tick,  work_event  },
    [SM_SLEEP] = { sleep_init, sleep_tick, sleep_event },
};

static sm_state_t current_state = SM_IDLE;

void sm_init(void) {
    current_state = SM_IDLE;
    if (sm_table[current_state].init) {
        sm_table[current_state].init();
    }
}

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

sm_state_t sm_get_state(void) {
    return current_state;
}

// 状态处理函数 - 空实现，后续填充
static void idle_init(void) {
    LOG_INFO("enter IDLE");
}

static void idle_tick(void) {
    // 预留
}

static void idle_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_WORK);
            break;
        case EVT_TIMER_1S:
            sm_jump(SM_SLEEP);
            break;
        default:
            break;
    }
}

static void work_init(void) {
    LOG_INFO("enter WORK");
}

static void work_tick(void) {
    // 预留
}

static void work_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_SLEEP);
            break;
        case EVT_KEY_LONG:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}

static void sleep_init(void) {
    LOG_INFO("enter SLEEP");
}

static void sleep_tick(void) {
    // 预留
}

static void sleep_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}
```

---

- [ ] **Step 5.3: Commit 状态机模块**

```bash
git add App/sm/sm.h App/sm/sm.c
git commit -m "feat(app): add state machine with 3 empty states"
```

---

### Task 6: 集成框架到 main.c

**目标：** 在主循环中集成所有框架模块

**Files:**
- Modify: `Core/Src/main.c:20-30` (includes)
- Modify: `Core/Src/main.c:65-103` (main function)
- Modify: `Core/Src/main.c:146-163` (添加定时器回调)

---

- [ ] **Step 6.1: 添加头文件引用**

在 `main.c` 第 20 行后添加：

```c
#include "event_queue.h"
#include "soft_timer.h"
#include "key.h"
#include "sm.h"
#include "log.h"
```

---

- [ ] **Step 6.2: 添加定时器回调函数**

在 `main.c` 的 USER CODE BEGIN 4 区域添加：

```c
static void on_timer_100ms(void) {
    evt_queue_post(EVT_TIMER_100MS, 0);
}

static void on_timer_1s(void) {
    evt_queue_post(EVT_TIMER_1S, 0);
}
```

---

- [ ] **Step 6.3: 修改 main() 函数**

替换整个 main() 函数：

```c
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG_Init();

  /* USER CODE BEGIN 2 */
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
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 框架主循环 */
    soft_timer_update();

    event_t evt;
    if (evt_queue_get(&evt) == 0) {
        sm_event(evt);
    }

    sm_tick();

    HAL_Delay(1);  // 1ms 循环周期
  }
  /* USER CODE END 3 */
}
```

---

- [ ] **Step 6.4: Commit main.c 集成**

```bash
git add Core/Src/main.c
git commit -m "feat(core): integrate framework into main loop"
```

---

### Task 7: 添加 GPIO 中断回调

**目标：** 配置 EXTI 中断回调，为按键驱动准备

**Files:**
- Modify: `Core/Src/stm32f1xx_it.c:200-203` (USER CODE BEGIN 1)

---

- [ ] **Step 7.1: 添加中断回调**

在 `stm32f1xx_it.c` 的 USER CODE BEGIN 1 区域添加：

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        key_exti_handler();
    }
}
```

---

- [ ] **Step 7.2: Commit 中断回调**

```bash
git add Core/Src/stm32f1xx_it.c
git commit -m "feat(core): add EXTI interrupt callback"
```

---

### Task 8: 更新 CMakeLists.txt

**目标：** 添加新源文件到编译

**Files:**
- Modify: `CMakeLists.txt`

---

- [ ] **Step 8.1: 读取当前 CMakeLists.txt**

查看当前的源文件列表位置。

---

- [ ] **Step 8.2: 添加源文件路径**

在 CMakeLists.txt 的源文件列表中添加：

```cmake
# Bsp sources
${CMAKE_SOURCE_DIR}/Bsp/event_queue.c
${CMAKE_SOURCE_DIR}/Bsp/soft_timer.c
${CMAKE_SOURCE_DIR}/Bsp/key.c
${CMAKE_SOURCE_DIR}/Bsp/log.c

# App sources
${CMAKE_SOURCE_DIR}/App/sm/sm.c
```

---

- [ ] **Step 8.3: 添加头文件搜索路径**

在 CMakeLists.txt 的 include_directories 中添加：

```cmake
${CMAKE_SOURCE_DIR}/Bsp
${CMAKE_SOURCE_DIR}/App/sm
```

---

- [ ] **Step 8.4: Commit CMakeLists.txt 更新**

```bash
git add CMakeLists.txt
git commit -m "build(cmake): add framework source paths"
```

---

### Task 9: 编译验证

**目标：** 确保所有模块编译通过

**Files:**
- 检查所有新建和修改的文件

---

- [ ] **Step 9.1: 运行 CMake 配置**

```bash
cd D:\Desktop\learn\KJ\LJ_KJ
cmake -B build
```

预期：配置成功，无错误

---

- [ ] **Step 9.2: 编译项目**

```bash
cmake --build build
```

预期：编译成功，无错误，生成固件

---

- [ ] **Step 9.3: 修复编译错误（如有）**

如果编译失败，检查：
1. 头文件路径是否正确
2. 源文件是否都添加到 CMakeLists.txt
3. 函数声明是否匹配

---

- [ ] **Step 9.4: Commit 最终版本**

```bash
git add -A
git commit -m "feat: complete state machine framework setup"
```

---

## 执行顺序

```
Task 1 (log) → Task 2 (event_queue) → Task 3 (soft_timer) → Task 4 (key) → Task 5 (sm) → Task 6 (main.c) → Task 7 (中断) → Task 8 (CMake) → Task 9 (编译)
```

**每个 Task 独立可测试，可按需中断。**

---

## 自检完成 ✓

- [x] 无 TBD/TODO 占位符
- [x] 所有类型、函数名一致
- [x] 覆盖设计文档所有模块
- [x] 文件路径准确
- [x] 代码块完整
