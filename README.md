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
