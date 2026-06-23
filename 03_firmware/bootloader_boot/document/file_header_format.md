# 文件头 & 函数注释格式规范

## 文件头模板

```c
/*  Copyright (c) <year> TimTan
 *  All rights reserved
 *
 *  File name: <module>.c
 *  Description: <一句话描述模块功能>
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  <date>               v01           TimTan        Create file
 *--------------------------------------------------
*/
```

示例：

```c
/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_gpio.c
 *  Description: BSP GPIO driver — LED initialization and control
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/
```

## 函数注释模板（关键模块）

```c
/*==============================================================================
 *  Function: <return_type> <func_name>(<params>)
 *  Input:  <param1> - <description>
 *          <param2> - <description>
 *  Output: none
 *  Return: <description>
 *  Description: <功能简述>
 *============================================================================*/
```

示例：

```c
/*==============================================================================
 *  Function: uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
 *  Input:  addr - destination address in flash
 *          buf  - source data buffer
 *          len  - number of bytes to write
 *  Output: none
 *  Return: 0 = success, 1 = address error, 2 = erase error, 3 = program error
 *  Description: Write data to flash within the last 2 pages.
 *               Automatically erases affected pages before writing.
 *============================================================================*/
```

## 简单函数注释（简短功能）

```c
/*
 *  Function: void bsp_led_toggle(void)
 *  Description: Toggle LED state.
*/
```

## 模块分隔注释

```c
/*==============================================================================
 * Local Variables
 *============================================================================*/

/*==============================================================================
 * CRC Functions
 *============================================================================*/
```

## 文件结尾

```c
// end of file
```

## 关键规则

- 多行注释使用 `/* ... */` 格式，不混用 `//`
- 行内注释使用 `//`，说明 WHY 不说明 WHAT
- 版权年份为文件创建年份
- 文件作者统一 `TimTan`
