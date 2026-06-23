# C 语言编码风格规范

> 基于 C_Coding_Style_Guide.txt 提取，适配 bootloader 项目

## 命名规则

| 对象 | 规则 | 示例 |
|------|------|------|
| 宏 `#define` | `ALL_CAPS`，下划线分隔 | `APP_TASK_MAX_NUM`, `BSP_UART_RX_BUF_SIZE` |
| 枚举类型 | `en_` 前缀 | `en_status_t`, `en_button_state_t` |
| 枚举值 | `ALL_CAPS` + 下划线 | `STATUS_OK`, `BUTTON_STATE_IDLE` |
| 结构体类型 | `stc_` 前缀 | `stc_button_t`, `stc_task_t` |
| 函数 | `snake_case`，模块前缀 | `bsp_led_init()`, `app_button_scan()` |
| 全局变量 | `snake_case`，`g_` 前缀 | `g_tick_ms`, `g_task_table` |
| 局部变量 | `snake_case` | `byte_count`, `prev_state` |
| 结构体成员 | `snake_case` | `.callback`, `.interval_ms` |
| 函数指针 | `snake_case`，`_cb_t` 或 `_t` 后缀 | `bsp_uart_rx_cb_t`, `app_task_cb_t` |

## 类型

- 必须使用 `<stdint.h>` 固定宽度类型：`uint8_t`, `uint16_t`, `uint32_t`, `int8_t`
- 布尔：`boolean_t` (typedef uint8_t)，配合 `TRUE` / `FALSE`
- 无符号常量加 `U` 后缀：`0xFFu`, `1000U`

## 格式

| 规则 | 说明 |
|------|------|
| 缩进 | 4 空格 |
| 函数大括号 | 左括号另起一行 |
| 控制语句大括号 | 左括号同行 `if (x) {` |
| 关键字后空格 | `if (`, `for (`, `while (` |
| 二元运算符空格 | `a = b + c` |
| 指针星号 | 靠变量名 `uint8_t *buf` |

## 模块前缀

| 层 | 前缀 | 示例 |
|----|------|------|
| Bsp 驱动 | `bsp_` | `bsp_uart_init()` |
| App 应用 | `app_` | `app_button_scan()` |
| Algorithm 算法 | `algo_` | `algo_crc_calc()` |

## static 限定

- 模块内部函数 → `static`
- 模块内部变量 → `static`
- 只有对外接口不加 `static`，在 .h 中声明

## 宏定义

```c
/* 常量：ALL_CAPS */
#define CELL_COUNT          8U
#define MEASURE_PERIOD      100U

/* 函数式宏：ALL_CAPS，参数加括号 */
#define MAXIMUM(x, y)       ((x) > (y) ? (x) : (y))

/* 位宏 */
#define BV(n)               ((uint32_t)1u << (n))
```

## 禁止魔法数字

- 所有阈值、尺寸、地址使用宏定义
- 例外：`0` 和 `1` 可直接使用

## 头文件保护

```c
#ifndef __MODULE_NAME_H
#define __MODULE_NAME_H
...
#endif /* __MODULE_NAME_H */
```

## 头文件包含顺序

1. 该模块的 .h 文件
2. 标准库 `<stdint.h>` `<stddef.h>`
3. 芯片厂商库 `stm32f1xx_hal.h`
4. BSP 层
5. App 层
6. Common 层

## goto

- 仅用于错误处理和统一退出，禁止自由跳转

## 比较运算

- 常量写在右侧：`if (ptr != NULL)`, `if (count >= MAX_NUM)`
