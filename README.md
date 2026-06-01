# Bare-Metal State Machine Framework

一个轻量级的 STM32 裸机状态机框架，采用事件驱动架构，支持软件定时器和 RTT 调试输出。

---

## 特性

- **事件驱动架构** - 中断通过事件队列与主循环通信
- **表驱动状态机** - 易于扩展新状态和事件
- **软件定时器** - 不占用硬件定时器，支持单次/重复模式
- **RTT 调试** - 通过 SEGGER RTT 输出日志，无需 UART
- **模块化设计** - BSP 层与应用层分离，便于移植

---

## 架构

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

---

## 目录结构

```
project_root/
├── Core/                       ← CubeMX 生成代码
│   ├── Inc/
│   └── Src/
│       ├── main.c              ← 框架集成入口
│       └── stm32f1xx_it.c      ← 中断回调
│
├── Bsp/                        ← 板级驱动层
│   ├── event_queue.h/c         ← 事件队列
│   ├── soft_timer.h/c          ← 软件定时器
│   ├── key.h/c                 ← 按键驱动
│   └── log.h/c                 ← RTT 日志
│
├── App/                        ← 应用层
│   └── sm/
│       ├── sm.h                ← 状态机接口
│       └── sm.c                ← 状态机实现
│
├── RTT/                        ← SEGGER RTT 库
├── Drivers/                    ← STM32 HAL 库
└── docs/                       ← 文档
```

---

## 移植指南

### 框架架构说明

本框架采用分层设计，便于移植：

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (App/)                         │
│  • 状态机逻辑                                            │
│  • 业务处理                                              │
│  • 平台无关，可直接复用                                   │
├─────────────────────────────────────────────────────────┤
│                    板级层 (Bsp/)                         │
│  • 事件队列                                              │
│  • 软件定时器                                            │
│  • 日志输出                                              │
│  • 需要少量适配                                          │
├─────────────────────────────────────────────────────────┤
│                    硬件层 (Core/)                        │
│  • HAL 库 / SDK                                         │
│  • 中断处理                                              │
│  • 需要根据芯片修改                                      │
└─────────────────────────────────────────────────────────┘
```

---

### 移植到空的 STM32 工程

#### **方法1：复制核心文件（推荐）**

**只需复制这些文件（与芯片无关）：**
```
Bsp/
├── event_queue.h    ← 复制
├── event_queue.c    ← 复制
├── soft_timer.h     ← 复制
├── soft_timer.c     ← 复制
└── log.h            ← 复制（需修改 RTT 头文件路径）

App/
└── sm/
    ├── sm.h         ← 复制
    ├── sm.c         ← 复制
    ├── state_idle.c ← 复制
    ├── state_work.c ← 复制
    └── state_sleep.c← 复制
```

**步骤：**

1. **复制文件到新工程**
```bash
cp -r Bsp/ /path/to/new_project/
cp -r App/ /path/to/new_project/
```

2. **配置 CMakeLists.txt**
```cmake
# 添加源文件
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Bsp/event_queue.c
    ${CMAKE_SOURCE_DIR}/Bsp/soft_timer.c
    ${CMAKE_SOURCE_DIR}/App/sm/sm.c
    ${CMAKE_SOURCE_DIR}/App/sm/state_idle.c
    ${CMAKE_SOURCE_DIR}/App/sm/state_work.c
    ${CMAKE_SOURCE_DIR}/App/sm/state_sleep.c
)

# 添加头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Bsp
    ${CMAKE_SOURCE_DIR}/App/sm
)
```

3. **集成到 main.c**
```c
#include "event_queue.h"
#include "soft_timer.h"
#include "sm.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // 框架初始化
    soft_timer_init();
    evt_queue_init();
    sm_init();
    
    // 创建定时器
    soft_timer_create(TIMER_REPEAT, 100, on_timer_100ms);
    soft_timer_create(TIMER_REPEAT, 1000, on_timer_1s);
    
    while (1) {
        soft_timer_update();
        
        event_t evt;
        if (evt_queue_get(&evt) == 0) {
            sm_event(evt);
        }
        
        sm_tick();
        HAL_Delay(1);
    }
}
```

4. **添加定时器回调（main.c）**
```c
static void on_timer_100ms(void) {
    evt_queue_post(EVT_TIMER_100MS, 0);
}

static void on_timer_1s(void) {
    evt_queue_post(EVT_TIMER_1S, 0);
}
```

5. **配置日志输出**

如果使用 RTT，复制 RTT/ 目录并修改 log.h 中的路径。

如果使用 UART，修改 log.h：
```c
#define LOG_INFO(fmt, ...) printf("[I] " fmt "\n", ##__VA_ARGS__)
```

---

#### **方法2：手动创建（适合学习）**

按照 [框架使用指南](docs/framework-usage-guide.md) 逐步创建各模块。

---

### 移植到其他芯片（非 STM32）

#### **通用移植原则**

框架的 **App/** 和 **Bsp/** 层大部分代码与芯片无关，只需修改以下部分：

| 文件 | 是否需要修改 | 说明 |
|------|-------------|------|
| **App/sm/** | ❌ 无需修改 | 状态机逻辑完全通用 |
| **Bsp/event_queue** | ❌ 无需修改 | 纯算法，与硬件无关 |
| **Bsp/soft_timer** | ⚠️ 少量修改 | 需要适配系统定时器 |
| **Bsp/log** | ⚠️ 少量修改 | 需要适配输出方式 |
| **Bsp/key** | ✅ 需要修改 | GPIO 驱动不同 |
| **main.c** | ✅ 需要修改 | 初始化和主循环 |

---

#### **示例1：移植到 ESP32 (ESP-IDF)**

**1. 复制文件**
```bash
cp -r Bsp/ /path/to/esp_project/components/my_bsp/
cp -r App/ /path/to/esp_project/main/
```

**2. 修改 soft_timer.c**
```c
// 替换 SysTick 为 ESP32 定时器
#include "esp_timer.h"

static uint32_t get_tick_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void soft_timer_update(void) {
    static uint32_t last_tick = 0;
    uint32_t now = get_tick_ms();
    
    if (now - last_tick < 1) {
        return;  // 不足1ms，跳过
    }
    last_tick = now;
    
    // 原有逻辑...
}
```

**3. 修改 log.h**
```c
#include "esp_log.h"

#define LOG_INFO(fmt, ...) ESP_LOGI("APP", fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  ESP_LOGE("APP", fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  ESP_LOGD("APP", fmt, ##__VA_ARGS__)
```

**4. 集成到 app_main.c**
```c
#include "event_queue.h"
#include "soft_timer.h"
#include "sm.h"

void app_main(void) {
    // 初始化
    soft_timer_init();
    evt_queue_init();
    sm_init();
    
    // 创建定时器
    soft_timer_create(TIMER_REPEAT, 100, on_timer_100ms);
    
    // 主循环（或使用 FreeRTOS 任务）
    while (1) {
        soft_timer_update();
        
        event_t evt;
        if (evt_queue_get(&evt) == 0) {
            sm_event(evt);
        }
        
        sm_tick();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

---

#### **示例2：移植到 nRF52 (nRF5 SDK)**

**1. 复制文件到工程**

**2. 修改 soft_timer.c**
```c
// 使用 nRF5 的 app_timer
#include "app_timer.h"

APP_TIMER_DEF(m_soft_timer_id);

static void soft_timer_handler(void *p_context) {
    soft_timer_update();
}

void soft_timer_init(void) {
    app_timer_init();
    app_timer_create(&m_soft_timer_id, APP_TIMER_MODE_REPEATED, soft_timer_handler);
    app_timer_start(m_soft_timer_id, APP_TIMER_TICKS(1));  // 1ms
}
```

**3. 修改 log.h**
```c
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define LOG_INFO(fmt, ...) NRF_LOG_INFO(fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  NRF_LOG_ERROR(fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  NRF_LOG_DEBUG(fmt, ##__VA_ARGS__)
```

**4. 集成到 main.c**
```c
#include "event_queue.h"
#include "soft_timer.h"
#include "sm.h"

int main(void) {
    // nRF5 初始化
    APP_SCHED_INIT(...);
    APP_TIMER_INIT(...);
    
    // 框架初始化
    soft_timer_init();
    evt_queue_init();
    sm_init();
    
    // 主循环
    while (1) {
        app_sched_execute();
        // soft_timer_update() 由定时器中断触发
        
        event_t evt;
        if (evt_queue_get(&evt) == 0) {
            sm_event(evt);
        }
        
        sm_tick();
        idle_state_handle();
    }
}
```

---

#### **示例3：移植到 RP2040 (Raspberry Pi Pico)**

**1. 复制文件**

**2. 修改 soft_timer.c**
```c
#include "pico/stdlib.h"
#include "hardware/timer.h"

static uint32_t get_tick_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void soft_timer_update(void) {
    // 使用硬件定时器或轮询方式
    static uint32_t last_tick = 0;
    uint32_t now = get_tick_ms();
    
    if (now - last_tick < 1) {
        return;
    }
    last_tick = now;
    
    // 原有逻辑...
}
```

**3. 修改 log.h**
```c
#include "pico/stdlib.h"

#define LOG_INFO(fmt, ...) printf("[I] " fmt "\n", ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)  printf("[E] " fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)  printf("[D] " fmt "\n", ##__VA_ARGS__)
```

**4. 集成到 main.c**
```c
#include "pico/stdlib.h"
#include "event_queue.h"
#include "soft_timer.h"
#include "sm.h"

int main(void) {
    stdio_init_all();
    
    // 框架初始化
    soft_timer_init();
    evt_queue_init();
    sm_init();
    
    // 主循环
    while (1) {
        soft_timer_update();
        
        event_t evt;
        if (evt_queue_get(&evt) == 0) {
            sm_event(evt);
        }
        
        sm_tick();
        sleep_ms(1);
    }
}
```

---

### 移植检查清单

#### **必需修改项**

- [ ] **系统定时器** - 实现 1ms 精度的 `get_tick_ms()` 或调用 `soft_timer_update()`
- [ ] **日志输出** - 适配目标平台的输出方式（UART/RTT/USB）
- [ ] **GPIO 驱动** - 如果使用按键，需修改 key.c
- [ ] **主循环集成** - 在 main() 中初始化框架并调用主循环函数

#### **可选修改项**

- [ ] **事件队列大小** - 根据需求调整 `EVT_QUEUE_SIZE`
- [ ] **定时器数量** - 根据需求调整 `SOFT_TIMER_MAX`
- [ ] **日志级别** - 可添加全局日志开关

#### **无需修改项**

- [x] **状态机逻辑** - App/sm/ 完全通用
- [x] **事件队列算法** - 纯软件实现
- [x] **状态表设计** - 与硬件无关

---

### 移植示例代码

完整的移植示例可在 `examples/` 目录找到（如果提供）。

---

## 快速开始

### 环境要求

- STM32CubeMX（可选，用于配置外设）
- CMake 3.15+
- GCC ARM 工具链（arm-none-eabi-gcc）
- SEGGER J-Link 或 ST-Link（用于烧录和调试）

### 编译

```bash
# 配置 CMake
cmake -B build

# 编译
cmake --build build
```

生成固件：`build/Debug/LJ_KJ.elf`

### 烧录

使用 J-Link：
```bash
JLinkExe -device STM32F103C8 -if SWD -speed 4000
> loadfile build/Debug/LJ_KJ.elf
> r
> g
```

使用 OpenOCD：
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/Debug/LJ_KJ.elf verify reset exit"
```

### 调试

1. 打开 SEGGER RTT Viewer
2. 选择 USB -> J-Link -> OK
3. 查看日志输出

---

## 使用示例

### 添加新状态

1. 在 `App/sm/sm.h` 中添加状态枚举：

```c
typedef enum {
    SM_IDLE = 0,
    SM_WORK,
    SM_SLEEP,
    SM_CONFIG,      // ← 新增
    SM_COUNT
} sm_state_t;
```

2. 在 `App/sm/sm.c` 中实现状态处理：

```c
static void config_init(void) {
    LOG_INFO("enter CONFIG");
}

static void config_tick(void) {
    // 周期处理
}

static void config_event(event_t evt) {
    switch (evt.id) {
        case EVT_KEY_SHORT:
            sm_jump(SM_IDLE);
            break;
        default:
            break;
    }
}
```

3. 在状态表中注册：

```c
static const sm_handler_t sm_table[SM_COUNT] = {
    [SM_IDLE]   = { idle_init,   idle_tick,   idle_event   },
    [SM_WORK]   = { work_init,   work_tick,   work_event   },
    [SM_SLEEP]  = { sleep_init,  sleep_tick,  sleep_event  },
    [SM_CONFIG] = { config_init, config_tick, config_event },
};
```

### 添加新事件

1. 在 `Bsp/event_queue.h` 中添加事件：

```c
typedef enum {
    EVT_NONE = 0,
    EVT_KEY_SHORT,
    EVT_KEY_LONG,
    EVT_TIMER_100MS,
    EVT_TIMER_1S,
    EVT_UART_RX,        // ← 新增
} event_id_t;
```

2. 在中断或定时器中投递：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    evt_queue_post(EVT_UART_RX, rx_data);
}
```

3. 在状态机中响应：

```c
static void work_event(event_t evt) {
    switch (evt.id) {
        case EVT_UART_RX:
            LOG_INFO("rx: %c", (char)evt.param);
            break;
        default:
            break;
    }
}
```

### 创建定时器

```c
// 定义回调
static void on_500ms(void) {
    evt_queue_post(EVT_TIMER_500MS, 0);
}

// 创建定时器（在 main.c 的 USER CODE BEGIN 2）
soft_timer_create(TIMER_REPEAT, 500, on_500ms);
```

---

## API 参考

### 事件队列

| 函数 | 说明 |
|------|------|
| `evt_queue_init()` | 初始化队列 |
| `evt_queue_post(id, param)` | 投递事件（中断安全） |
| `evt_queue_get(out)` | 获取事件（主循环调用） |

### 软件定时器

| 函数 | 说明 |
|------|------|
| `soft_timer_init()` | 初始化定时器模块 |
| `soft_timer_create(mode, ms, cb)` | 创建定时器 |
| `soft_timer_start(id)` | 启动定时器 |
| `soft_timer_stop(id)` | 停止定时器 |

### 状态机

| 函数 | 说明 |
|------|------|
| `sm_init()` | 初始化状态机 |
| `sm_event(evt)` | 投递事件给状态机 |
| `sm_tick()` | 周期处理 |
| `sm_jump(state)` | 切换状态 |
| `sm_get_state()` | 获取当前状态 |

### 日志

```c
LOG_INFO(fmt, ...)  // [I] 信息日志
LOG_ERR(fmt, ...)   // [E] 错误日志
LOG_DBG(fmt, ...)   // [D] 调试日志
```

---

## 硬件支持

### 目标芯片

- STM32F103C8T6（或其他 F103 系列）

### 已配置外设

- GPIO（LED、按键）
- IWDG（看门狗，4秒超时）
- SysTick（系统定时器）

### 可扩展外设

- UART（串口通信）
- SPI（传感器、显示屏）
- I2C（传感器）
- ADC（模拟采集）

---

## 注意事项

1. **CubeMX 代码保护** - 自定义代码放在 `USER CODE BEGIN/END` 区域
2. **中断安全** - 事件队列可中断调用，定时器回调在主循环
3. **队列容量** - 默认16，事件过快可能丢失
4. **看门狗** - 已启用 IWDG，主循环要及时喂狗

---

## 文档

- [框架使用指南](docs/framework-usage-guide.md) - 详细的使用说明
- [STM32 框架指南](STM32_Framework_Guide.md) - 原始设计文档

---

## 资源

- [STM32F103 参考手册](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-arm-based-32-bit-mcus-microelectronics-stmicroelectronics.pdf)
- [SEGGER RTT 文档](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer/)
- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html)

---

## 许可证

MIT License

---

## 作者

[chen111111111112121](https://github.com/chen111111111112121)

---

## 贡献

欢迎提交 Issue 和 Pull Request！
