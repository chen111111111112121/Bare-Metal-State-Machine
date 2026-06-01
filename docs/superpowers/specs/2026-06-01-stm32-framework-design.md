# STM32 状态机框架设计

> 日期：2026-06-01
> 基于 STM32_Framework_Guide.md 实现

---

## 1. 项目目标

搭建通用裸机状态机框架，支持：
- 事件驱动架构（中断→队列→主循环）
- 软件定时器（不占用硬件定时器）
- RTT 调试输出
- 可扩展的状态机设计

---

## 2. 架构设计

### 2.1 目录结构

```
project_root/
├── Core/                       ← CubeMX 生成（不改动）
├── Bsp/                        ← 板级驱动层
│   ├── event_queue.h/c         ← 事件队列
│   ├── soft_timer.h/c          ← 软件定时器
│   ├── key.h/c                 ← 按键驱动（桩）
│   └── log.h/c                 ← RTT 日志宏
└── App/                        ← 应用层
    └── sm/
        ├── sm.h                ← 状态机接口
        └── sm.c                ← 状态机实现
```

### 2.2 层次关系

```
中断 (EXTI/SysTick)
    ↓ post
Bsp/event_queue
    ↓ get
App/sm（主循环中处理）
```

- **Bsp 层**：硬件相关，中断安全
- **App 层**：业务逻辑，主循环中执行
- **事件队列**：中断→主循环的桥梁

---

## 3. 模块规格

### 3.1 事件队列

**接口：**
```c
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
int  evt_queue_post(event_id_t id, uint32_t param);  // 中断中调用
int  evt_queue_get(event_t *out);                      // 主循环中调用
```

**实现：**
- 环形缓冲区，大小 16
- `post` 队列满返回 -1，不阻塞
- `get` 队列空返回 -1

---

### 3.2 软件定时器

**接口：**
```c
#define SOFT_TIMER_MAX 8

typedef enum { TIMER_ONCE, TIMER_REPEAT } timer_mode_t;
typedef void (*timer_cb_t)(void);

uint32_t soft_timer_create(timer_mode_t mode, uint32_t ms, timer_cb_t cb);
void     soft_timer_update(void);   // 主循环中调用
```

**实现：**
- SysTick 中递减计数，主循环中检查超时并回调
- 回调中可投递事件到队列

---

### 3.3 状态机

**接口：**
```c
typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_COUNT
} sm_state_t;

void sm_init(void);
void sm_event(event_t evt);
void sm_tick(void);
void sm_jump(sm_state_t new_state);
```

**实现：**
- 状态表驱动（函数指针数组）
- 每个状态有 3 个处理函数：init、tick、event
- 初始状态：SM_IDLE

**状态处理模式：**
```c
static void idle_init(void) { /* 进入时初始化 */ }
static void idle_tick(void) { /* 周期检查 */ }
static void idle_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_WORK);
            break;
        default:
            break;
    }
}
```

---

### 3.4 按键驱动（暂留桩）

**接口：**
```c
void key_init(void);
void key_poll(void);
void key_exti_handler(void);
```

**实现：** 暂时空函数，后续扩展。

---

### 3.5 调试日志

**接口：**
```c
#define LOG_INFO(fmt, ...) SEGGER_RTT_printf(0, "[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  SEGGER_RTT_printf(0, "[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  SEGGER_RTT_printf(0, "[D] " fmt "\n", ##__VA_ARGS__)
```

**实现：** 纯宏定义，log.c 可为空文件。

---

## 4. 主循环流程

```c
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_IWDG_Init();
    
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

---

## 5. 扩展指南

### 添加新状态

1. 在 `sm.h` 枚举中添加状态（SM_COUNT 之前）
2. 在 `sm.c` 中实现 `xxx_init/tick/event` 函数
3. 在 `sm_table` 中注册

### 添加新事件

1. 在 `event_queue.h` 枚举中添加事件
2. 在需要的状态处理函数中响应
3. 在中断或定时器回调中投递事件

### 添加新模块

1. 在 `Bsp/` 中创建模块文件
2. 在 `main.c` 中初始化
3. 通过事件队列与主循环通信

---

## 6. API 快速参考

### nRF → STM32

| nRF | STM32 HAL |
|-----|-----------|
| `nrf_gpio_pin_set(pin)` | `HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_SET)` |
| `nrf_gpio_pin_clear(pin)` | `HAL_GPIO_WritePin(GPIOx, pin, GPIO_PIN_RESET)` |
| `nrf_gpio_pin_read(pin)` | `HAL_GPIO_ReadPin(GPIOx, pin)` |
| `nrf_delay_ms(ms)` | `HAL_Delay(ms)` |
| `NRF_LOG_INFO(...)` | `LOG_INFO(...)` |
| `app_timer_create/start/stop` | `soft_timer_create/start/stop` |

---

## 7. 注意事项

- CubeMX 生成的代码不要修改（除 main.c 和中断文件）
- 中断回调放在 `stm32f1xx_it.c` 或单独文件
- RTT 库已移植，可直接使用 `SEGGER_RTT_printf`
- IWDG 看门狗已启用，主循环要及时喂狗
