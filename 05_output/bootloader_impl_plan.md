# Bootloader 实施计划 — STM32F103RCT6

> 本文档由 CLI Claude 负责维护更新，VS Code Claude 按此计划逐项实现。

## 项目概述

| 项 | 内容 |
|:---|:---|
| MCU | STM32F103RCT6 (256KB Flash, 48KB RAM) |
| 通信 | USART2 (PA2=TX, PA3=RX), 115200-8-N-1 |
| 库 | STM32F1xx HAL |
| 编译 | Keil MDK-ARM v5, Microlib |
| 上位机 | C# WinForm .NET 4.8 |

## Flash 分区

```
0x08000000 ─ 0x08001FFF  Bootloader (8KB, Sector 0 前半)
0x08002000 ─ 0x08003FFF  参数区    (8KB, Sector 0 后半)
0x08004000 ─ 0x0803FFFF  APP       (240KB, Sector 1-5)
```

## 参考源码

| 项目 | 路径 | 学习要点 |
|:---|:---|:---|
| Balance_Car (用户自己的) | `01_input/balance_car_uart_ref/` | USART2 HAL 驱动，已验证硬件可用 |
| simpleboot | `01_input/simpleboot/` | HAL 完整 bootloader 架构，Y-Modem 协议 |
| STM32F103C8-Basic-Bootloader | `01_input/STM32F103C8-Basic-Bootloader/` | 自定义命令协议 + Python 上位机 |
| temp 工程 (已验证) | `03_firmware/temp/` | 最小可用 HAL UART echo |

---

## 任务列表

### Phase 1 — 固件框架（可编译、可收发）

#### 任务 1.1：清理 bootloader SDK 目录

- 删除 `sdk/` 下所有旧的 CMSIS 头文件（core_cm3.h 等）
- 从 Balance_Car `SDK/` 目录完整复制 CMSIS + HAL 到 bootloader `sdk/`
- 确保 `stm32f1xx_hal_conf.h` 只开启需要的模块：UART, RCC, GPIO, CORTEX, FLASH
- 更新 `bootloader.uvprojx` 的 include path 和源文件列表

**验证标准**：Keil 编译 0 error 0 warning

#### 任务 1.2：实现 HAL 版 bsp_uart.c

- 完全照搬 Balance_Car `Bsp/bsp_uart.c` 的实现方式
- 用 `HAL_UART_Init`, `HAL_UART_Transmit`, `HAL_UART_Receive_IT` + ring buffer
- API 保持不变：`bsp_uart_init()`, `bsp_uart_send_byte()`, `bsp_uart_send_bytes()`, `bsp_uart_recv_byte()`, `bsp_uart_bytes_available()`

**验证标准**：集成进 temp 工程，echo 测试通过

#### 任务 1.3：实现 HAL 版 bsp_gpio.c

- LED0=PC8, LED1=PC9, KEY1=PB2
- 用 `HAL_GPIO_Init` 替代寄存器操作
- API：`bsp_gpio_init()`, `bsp_led_on/off/toggle()`, `bsp_led_all_on/off/toggle()`, `bsp_key_is_pressed()`

**验证标准**：单测 LED 闪烁 + 按键检测

#### 任务 1.4：实现 HAL 版 bsp_systick.c

- 用 `HAL_Init()` + `HAL_Delay()` 替代自定义 SysTick
- 保留 `bsp_systick_get_ms()` 兼容现有代码（内部读 `HAL_GetTick()`）

**验证标准**：`bsp_systick_get_ms()` 按时递增

#### 任务 1.5：实现 HAL 版 bsp_flash.c

- 用 `HAL_FLASH_Unlock/Lock`, `HAL_FLASHEx_Erase`, `HAL_FLASH_Program` 替代寄存器操作
- API 不变：`bsp_flash_unlock/lock()`, `bsp_flash_erase_sector()`, `bsp_flash_erase_app_area()`, `bsp_flash_program_halfwords()`

**验证标准**：读写参数区 0x08002000 正常

### Phase 2 — 协议层

#### 任务 2.1：定义通信协议

参考 STM32F103C8-Basic-Bootloader 的协议设计，但简化：

```
帧格式: [长度 1B][命令 1B][数据 N B][CRC16 2B]
长度 = 数据字节数 + 4（命令+CRC）

命令:
  0x01 PING    → 回复版本号
  0x02 ERASE   → 擦除 APP 区 → 回复状态
  0x03 WRITE   → 写 256B 数据块 → 回复状态
  0x04 VERIFY  → CRC 校验 → 回复状态
  0x05 RESET   → 软复位
```

**为什么不用我们之前的 AA/55 帧格式**：参考代码用的是更简单紧凑的格式，CRC 校验足够保证完整性

**验证标准**：协议文档通过审批

#### 任务 2.2：实现 app_protocol.c

- 接收状态机：收长度 → 收数据+CRC → 验证 → 回调处理
- 用 `bsp_uart_bytes_available()` / `bsp_uart_recv_byte()` 从 ring buffer 读数

**验证标准**：上位机发 PING，MCU 回版本号

### Phase 3 — 应用层

#### 任务 3.1：实现 app_boot.c

- 主流程：INIT → 检查触发条件 → 等待/下载 → 跳转 APP
- 触发条件：按键 5s 长按，或 APP 请求升级标志，或 3 秒内串口收到命令
- 参考 simpleboot 的状态机设计

**验证标准**：上电 LED 闪烁 → 上位机连接 → 握手成功

### Phase 4 — 上位机

#### 任务 4.1：实现 BootProtocol.cs

- 协议封装：与 MCU 端 app_protocol.c 对等
- 串口通信：`System.IO.Ports.SerialPort`，115200
- 帧收发：构建→发送→等待→CRC 验证→解析

**验证标准**：单元测试通过

#### 任务 4.2：实现 MainForm.cs

- COM 口选择 + 刷新
- 选择 .bin 固件文件
- 显示进度条（擦除→写入→校验）
- 操作日志（时间戳 + 彩色）
- 编码风格遵循 `CS_Coding_Style_Guide.txt`

**验证标准**：完整的固件下载流程跑通

### Phase 5 — 集成测试

#### 任务 5.1：端到端测试

- 编译 bootloader → 烧录到 0x08000000
- 编译 APP 模板 → 产生 app.bin
- 上位机选 COM 口 → 选 app.bin → 下载
- 验证 APP 自动启动，LED 双灯闪烁

---

## 编码规范

- **C 代码**：遵循 `D:/07_claude/C_Coding_Style_Guide.txt`
- **C# 代码**：遵循 `D:/07_claude/CS_Coding_Style_Guide.txt`
- **HAL 驱动代码**（.c/.h 来自 ST 官方）：不改风格
- 函数声明空参数必须加 `void`
- 枚举前缀 `en_`，结构体前缀 `stc_`
- 缩进 4 空格

## 注意

- ⚠ 不要混用 HAL 和裸寄存器访问同一个外设
- ⚠ 每完成一个任务，确认编译 0 error
- ⚠ 遇到不确定的硬件参数（引脚号、时钟频率），参考 Balance_Car 或问用户
- ⚠ 本文档的进度状态由 CLI Claude 更新
