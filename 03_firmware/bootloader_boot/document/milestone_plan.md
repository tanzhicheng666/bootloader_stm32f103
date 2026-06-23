# Bootloader 里程碑规划

> 创建日期: 2026-06-22  
> 芯片: STM32F103RCT6, 256KB Flash, 48KB RAM  
> 原则: 每个里程碑独立可测试，Tim 实测通过后进入下一个  
> 约束: 主循环中禁止阻塞延时，所有周期性工作通过任务调度器驱动

---

## Flash 分区

```
256KB Flash (0x08000000 ~ 0x0803FFFF)
┌────────────┬──────────┬──────────────────────────┐
│  Bootloader│  参数区   │         APP              │
│   16KB     │   2KB    │        238KB             │
│  页 0~7    │   页 8   │       页 9~127            │
│            │  (用1KB) │                          │
└────────────┴──────────┴──────────────────────────┘
0x08000000   0x08004000  0x08004800                 0x08040000
```

---

## M0: 工程框架 + LED 翻转 ✅ 已完成

**目标**: 验证工程结构、SDK、编译工具链正确

**文件**:
- `common/common_types.h`, `common/common_defines.h`
- `bsp/bsp_sysclk.c/h`, `bsp/bsp_tick.c/h`, `bsp/bsp_gpio.c/h`
- `core/stm32f1xx_hal_conf.h`, `core/stm32f1xx_it.c/h`
- `main/main.c/h`
- `sdk/` (CMSIS + HAL + LL 完整)
- `project/bootloader_boot.uvprojx`

**测试**: LED 以 500ms 间隔闪烁（使用阻塞延时）

---

## M1: 任务调度器 + 按键

**目标**: 
1. 建立任务调度框架，消除主循环中的阻塞延时
2. 添加按键驱动，实现按键检测与 LED 联动

### 新增文件

| 文件 | 说明 |
|------|------|
| `app/app_task.c/h` | 任务调度器：回调表 + 间隔 + 时间基准，最多 8 个任务 |
| `app/app_button.c/h` | 按键状态机：四态消抖（约 30ms），press+release 事件检测 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `bsp/bsp_gpio.c/h` | 新增 KEY 引脚定义（PB10=LEFT, PB2=RIGHT）+ `bsp_key_init()` + `bsp_key_left_read()` + `bsp_key_right_read()` |
| `main/main.c` | 从阻塞延时改为任务调度模式：`app_task_create()` 注册 LED 任务和按键扫描任务 → `while(1) { app_task_run(); }` |

### 实现要点

**app_task**:
- `app_task_init()` — 清空任务表
- `app_task_create(callback, interval_ms)` — 注册回调，返回任务 ID，-1 表示表满
- `app_task_run()` — 遍历任务表，到期任务调用 callback
- 时间基准：`bsp_tick_get()`，不直接调 `HAL_GetTick()`
- 首次执行：注册后立即运行一次，之后按间隔执行

**app_button**:
- 状态机四态：IDLE → PRESS_DEBOUNCE → PRESSED → RELEASE_DEBOUNCE → IDLE
- 消抖阈值：3 次 × 10ms = 30ms
- `app_button_scan()` — 每 10ms 调用一次
- `app_button_get_key_left()` / `app_button_get_key_right()` — 读取并清除事件标志

**main 结构**:
```c
int main(void)
{
    HAL_Init();
    bsp_sysclk_config();
    bsp_tick_init();
    bsp_led_init();
    bsp_key_init();

    app_button_init();
    app_task_init();

    /* 注册任务: 按键扫描 10ms, LED 翻转 500ms */
    app_task_create(task_button_scan, 10);
    app_task_create(task_led_blink, 500);

    while (1) {
        app_task_run();
    }
}
```

### 测试方法

1. 上电后 LED 以 500ms 间隔自动闪烁
2. 按下 KEY_LEFT → LED 立即翻转一次（响应按键事件）
3. 按下 KEY_RIGHT → LED 快速闪烁 3 次（另一种响应）
4. 长时间运行 → LED 节奏稳定，无阻塞卡顿
5. 可在 task_button_scan 中加计数器，验证 10ms 调用频率

### Keil 更新

- 新增 `app` 组：app_task.c、app_button.c
- bsp_gpio.c/h 已存在，仅修改

---

## M2: UART 收发 + FIFO

**目标**: 串口物理层打通，能收发任意字节

### 新增文件

| 文件 | 说明 |
|------|------|
| `algorithm/algo_fifo.c/h` | 通用 FIFO 队列：`algo_fifo_init()`, `algo_fifo_put()`, `algo_fifo_get()`, `algo_fifo_available()`, `algo_fifo_is_empty()`, `algo_fifo_is_full()` |
| `bsp/bsp_uart.c/h` | USART2 中断收发：TX/RX 环形缓冲，`bsp_uart_send()`, `bsp_uart_send_byte()`, `bsp_uart_available()`, `bsp_uart_read()`, RX 字节回调 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `core/stm32f1xx_it.c` | 新增 `USART2_IRQHandler()` → `HAL_UART_IRQHandler()` |
| `core/stm32f1xx_it.h` | 声明 `USART2_IRQHandler` |
| `main/main.c` | 新增 `bsp_uart_init(115200)` + UART echo 任务 |

### 实现要点

**bsp_uart**:
- USART2：PA2=TX, PA3=RX，115200-8-N-1
- 接收：`HAL_UART_Receive_IT()` 逐字节接收 → RX 环形缓冲
- 发送：TX 环形缓冲 + `HAL_UART_Transmit_IT()` 逐字节发送
- 错误处理：`HAL_UART_ErrorCallback()` 清标志 + 重启接收
- `bsp_uart_available()` 返回可读字节数

**algo_fifo**:
- 环形缓冲实现，大小可配置（如 256 字节）
- 独立于 bsp_uart，可被任何模块复用

### 测试方法

1. 上电后发送 "BOOT READY\r\n"
2. 串口助手发送任意字节 → 回环发送相同字节
3. 连续发送 256 字节 → 不丢字节
4. 发送速度 115200bps 持续 10 秒 → 无溢出

### Keil 更新

- 新增 `algorithm` 组：algo_fifo.c
- 新增 `bsp` 组：bsp_uart.c

---

## M3: 通信协议

**目标**: 帧解析 + CRC 校验 + 命令分发，PING/PONG 握手可测

### 新增文件

| 文件 | 说明 |
|------|------|
| `algorithm/algo_crc.c/h` | CRC-16/MODBUS 查表实现：`algo_crc16(data, len)` |
| `app/app_comm.c/h` | 帧解析状态机 + 命令处理函数表 + `app_comm_send_frame()` |

### 修改文件

| 文件 | 改动 |
|------|------|
| `main/main.c` | `app_comm_init()` → 注册 PING handler → 创建 comm 处理任务（1ms） |

### 协议帧格式

```
标准帧: [Length 1B][CMD 1B][Data N B][CRC16 2B]
         Length = CMD(1) + Data(N) + CRC(2) = N + 3
         Length 范围: 3 ~ 255

特殊帧: WRITE_DATA (CMD=0x0A) — 无 Length 字节
         [CMD=0x0A 1B][Data 256B][CRC16 2B]  固定 259 字节

CRC: CRC-16/MODBUS, 多项式 0x8005, 初始值 0xFFFF, LSB-first
     CRC 覆盖范围: CMD + Data
     发送时低字节在前 (little-endian)
```

### 命令表

| 命令 | 代码 | 方向 | 数据 | 说明 |
|------|------|------|------|------|
| PING | 0x01 | PC→MCU | 无 | 握手检测 |
| PONG | 0x02 | MCU→PC | version[2] | PING 应答，版本号 |
| ERASE | 0x03 | PC→MCU | 无 | 擦除参数页 |
| ERASE_ACK | 0x04 | MCU→PC | status[1] | 擦除应答 |
| WRITE_ADDR | 0x05 | PC→MCU | addr[4] LE | 写入起始地址 |
| ACK | 0x06 | MCU→PC | status[1] | 通用应答 |
| WRITE_DATA | 0x0A | PC→MCU | data[256] | 256B 数据块 |
| VERIFY | 0x07 | PC→MCU | addr[4]+len[2]+crc[2] | 校验请求 |
| VERIFY_ACK | 0x08 | MCU→PC | status[1] | 校验结果 |
| RESET | 0x09 | PC→MCU | 无 | 软件复位 |

### 实现要点

**app_comm**:
- 状态机三态：IDLE → RECV_FRAME / RECV_WRITE → 校验 CRC → 分发 → IDLE
- `app_comm_register_handler(cmd, callback)` — 注册命令处理函数
- `app_comm_process_byte(byte)` — 逐字节输入解析器
- `app_comm_send_frame(cmd, data, len)` — 构建帧 + 发送
- CRC 错误 → 静默丢弃帧，不发送错误响应（防止错误风暴）

**PING 处理**:
- 收到 PING → 返回 PONG + version[2]（主版本 + 次版本）
- version = 0x0100

### 测试方法

1. 串口助手发送 `05 01 7B 20` → 收到 `05 02 01 00 XX XX`
2. 发送 `05 01 00 00` → 收到带正确 CRC 的 PONG 帧
3. 发送错误 CRC → 无响应
4. 发送 Length=0 → 无响应
5. 发送 Length=2（小于最小值3）→ 无响应

### Keil 更新

- `algorithm` 组新增：algo_crc.c
- `app` 组新增：app_comm.c

---

## M4: Flash 驱动

**目标**: 参数区 Flash 读写擦，保护 bootloader 区不被误写

### 新增文件

| 文件 | 说明 |
|------|------|
| `bsp/bsp_flash.c/h` | Flash 读写擦，仅允许操作末 2 页（参数区） |

### Flash 参数 (STM32F103RCT6)

```c
#define BSP_FLASH_BASE              0x08000000U
#define BSP_FLASH_SIZE              0x00040000U   /* 256KB */
#define BSP_FLASH_PAGE_SIZE         0x00000800U   /* 2KB */
#define BSP_FLASH_PAGE_COUNT        128U
```

### 实现要点

- `bsp_flash_read(addr, buf, len)` — 内存映射直接读，支持任意地址
- `bsp_flash_write(addr, buf, len)` — 半字编程（16bit），写入前自动擦除，仅限末 2 页
- `bsp_flash_erase_page(page_addr)` — 单页擦除，仅限末 2 页
- 地址校验：写/擦超出末 2 页 → 返回错误码
- 字节对齐：写入长度奇数时，末字节补 0xFF
- 回读校验：写入后自动读回比对

### 测试方法

1. 调试器观察参数区初始值 → 全 0xFF（擦除态）
2. 通过串口命令或按键触发：写 4 字节 → 读回 → 比对一致
3. 写 256 字节跨页数据 → 读回一致
4. 尝试写 bootloader 区（0x08000000）→ 返回错误，数据未被修改
5. 擦除参数页 → 读回全 0xFF

### Keil 更新

- `bsp` 组新增：bsp_flash.c

---

## M5: Boot 逻辑 + APP 跳转

**目标**: 实现启动决策、升级标志管理、APP 跳转

### 新增文件

| 文件 | 说明 |
|------|------|
| `app/app_boot.c/h` | 启动原因检测 + APP 有效性检查 + 跳转执行 |

### 修改文件

| 文件 | 改动 |
|------|------|
| `main/main.c` | 初始化后调用 `app_boot_check()`，根据返回值进入升级模式或跳转 APP |
| `bsp/bsp_flash.c/h` | 新增参数区读写接口（读写升级标志、APP 大小、APP CRC） |

### 启动流程

```
上电
 → HAL_Init → bsp_sysclk_config → bsp_tick_init
 → bsp_led_init → bsp_key_init → app_button_init
 → app_boot_check():
     ├─ 按键按下(LEFT)? ──────────→ 返回 BOOT_MODE_UPGRADE
     ├─ 参数区 upgrade_flag=1? ──→ 返回 BOOT_MODE_UPGRADE
     └─ 以上都不满足:
          ├─ app_boot_is_app_valid()?
          │    ├─ 读 APP 起始地址栈指针值
          │    ├─ SP 在 RAM 范围(0x20000000~0x2000C000)?
          │    └─ 复位向量在 Flash 范围(0x08000000~0x0803FFFF)?
          ├─ 有效 → app_boot_jump_to_app()  (不返回)
          └─ 无效 → 返回 BOOT_MODE_UPGRADE

进入升级模式:
 → bsp_uart_init(115200)
 → app_comm_init()
 → app_task_init()
 → 注册 comm 任务 + LED 任务
 → while(1) { app_task_run(); }

跳转 APP:
 → __disable_irq()
 → SysTick->CTRL = 0
 → 所有用到的外设 DeInit 或复位
 → 设置 MSP = *(uint32_t *)APP_ADDR
 → 跳转到 *(uint32_t *)(APP_ADDR + 4)
```

### 参数区布局 (页 4, 前 1KB)

```
Offset 0x00: upgrade_flag   (4B)  0=正常, 0x5A5AA5A5=有待升级固件
Offset 0x04: app_size       (4B)  待升级固件大小 (字节)
Offset 0x08: app_crc        (4B)  待升级固件 CRC-32 或 CRC-16
Offset 0x0C: reserved       ...
```

### 测试方法

1. 默认上电（无按键 + 无升级标志 + 无 APP）→ LED 指示"进入升级模式"
2. 按住 KEY_LEFT 上电 → 进入升级模式（LED 特定闪烁）
3. 用调试器在参数区写 upgrade_flag=0x5A5AA5A5 → 复位 → 进入升级模式
4. 烧录一个简单 APP（LED 不同频率闪烁）到 0x08004800 → 上电 → 自动跳转 APP，APP LED 运行
5. APP 运行中 → 按下 KEY_LEFT + KEY_RIGHT 组合 → 写参数区 + 软复位 → 进入升级模式

### Keil 更新

- `app` 组新增：app_boot.c

---

## M6: 完整 IAP 升级流程

**目标**: 端到端固件升级 — 擦除 → 写入 → 校验 → 跳转 APP

### 修改文件

| 文件 | 改动 |
|------|------|
| `main/main.c` | 补全所有命令处理函数（ERASE, WRITE_ADDR, WRITE_DATA, VERIFY, RESET） |
| `app/app_boot.c` | 升级完成后写标志，校验通过后清标志，支持 RESET 命令 |

### 命令处理函数详细设计

```
comm_handler_ping(data, len):
    resp[0] = STATUS_OK
    resp[1] = VERSION & 0xFF      // 版本低字节
    resp[2] = VERSION >> 8         // 版本高字节
    app_comm_send_frame(CMD_PONG, resp, 3)

comm_handler_erase(data, len):
    bsp_flash_erase_page(PARAM_PAGE_ADDR)
    status = (erase_result == 0) ? STATUS_OK : STATUS_ERR
    app_comm_send_frame(CMD_ERASE_ACK, &status, 1)

comm_handler_write_addr(data, len):
    g_write_addr = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)
    status = (g_write_addr 在 APP 区范围内) ? STATUS_OK : STATUS_ERR
    app_comm_send_frame(CMD_ACK, &status, 1)

comm_handler_write_data(data, len):   // len = 256
    result = bsp_flash_write(g_write_addr, data, 256)
    status = (result == 0) ? STATUS_OK : STATUS_ERR
    g_write_addr += 256
    app_comm_send_frame(CMD_ACK, &status, 1)

comm_handler_verify(data, len):
    // data = addr[4] + verify_len[4] + expected_crc[4]  (12 bytes total)
    read_len    = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24)
    expected_crc = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24)
    // 读 flash，计算 CRC，与 expected_crc 比对
    status = (crc_match) ? STATUS_OK : STATUS_ERR
    app_comm_send_frame(CMD_VERIFY_ACK, &status, 1)

comm_handler_reset(data, len):
    status = STATUS_OK
    app_comm_send_frame(CMD_ACK, &status, 1)
    bsp_tick_delay_ms(10)    // 允许 ACK 发送完成（仅此处可用短阻塞延时）
    NVIC_SystemReset()
```

### 升级流程

```
PC                              MCU
 │                               │
 ├─ PING ──────────────────────→│ 握手，确认 bootloader 在线
 │←────────────── PONG + ver ──┤
 │                               │
 ├─ ERASE ─────────────────────→│ 擦除参数区（清除旧标志和数据）
 │←────────────── ERASE_ACK ────┤
 │                               │
 ├─ WRITE_ADDR(0x08004800) ────→│ 设置写入起始地址
 │←────────────── ACK ──────────┤
 │                               │
 ├─ WRITE_DATA(256B) ──────────→│ 写入第 1 块 256B
 │←────────────── ACK ──────────┤
 │         ... 重复 N 次 ...     │
 │                               │
 ├─ WRITE_DATA(256B) ──────────→│ 写入最后一块
 │←────────────── ACK ──────────┤
 │                               │
 ├─ VERIFY ─────────────────────→│ 全固件 CRC 校验
 │←────────────── VERIFY_ACK ───┤
 │                               │
 ├─ RESET ─────────────────────→│ 软件复位
 │                               │ → 启动 → 检测升级标志
 │                               │ → 标志=1? → 写标志=0
 │                               │ → 跳转 APP
```

### 测试方法

1. 编写一个简单的 APP 固件（LED 以 200ms 快速闪烁，区别于 bootloader 的 500ms）
2. 上位机通过串口发送 PING → 确认 bootloader 在线
3. 发送 ERASE → 确认擦除成功
4. 逐块发送 WRITE_DATA（APP 固件二进制）→ 每块确认 ACK
5. 发送 VERIFY → 确认 CRC 匹配
6. 发送 RESET → MCU 复位 → 自动跳转 APP → APP LED 快速闪烁
7. 再次断电上电 → 无升级标志 → 直接跳转 APP（LED 快速闪烁）

---

## 里程碑总览

| 里程碑 | 内容 | 新增文件数 | 可测试点 |
|--------|------|-----------|---------|
| M0 ✅ | 工程框架 + LED 阻塞翻转 | 13 | LED 闪烁 |
| M1 | 任务调度器 + 按键 | 4 | LED 任务闪烁 + 按键响应 |
| M2 | UART 收发 + FIFO | 4 | 串口回环、连续收发不丢字节 |
| M3 | 通信协议 | 4 | PING/PONG 握手，CRC 校验 |
| M4 | Flash 驱动 | 2 | 末 2 页读写擦，地址保护验证 |
| M5 | Boot 逻辑 + APP 跳转 | 2 | 按键/标志进入升级，跳转 APP |
| M6 | 完整 IAP 升级流程 | 0（修改） | 端到端固件升级 |

---

## 约定

- 每个里程碑开始前：先读 GitHub 参考实现 → 讨论 → Tim 拍板 → 写代码
- 每个里程碑完成后：Tim 编译 + 实测 + 确认 → 进入下一个
- 代码风格：遵循 `coding_style.md`
- 文件头格式：遵循 `file_header_format.md`
- 主循环中**禁止阻塞延时**（bsp_tick_delay_ms 仅限初始化阶段和复位前使用）
- App 层通过 Bsp 接口访问硬件，**禁止直接调用 HAL**
- App 层时间基准使用 `bsp_tick_get()`，**禁止直接调用 HAL_GetTick()**
