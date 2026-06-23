# Bootloader 通信协议

> 适用于 STM32F103RCT6 与 C# 上位机之间的 UART 通信

## 物理层

| 参数 | 值 |
|:---|:---|
| 接口 | USART2 |
| 引脚 | PA2=TX, PA3=RX |
| 波特率 | 115200 bps |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控 | 无 |

## 帧结构

### 上位机 → MCU（命令帧）

```
┌────────┬────────┬───────────────────┬────────┐
│ Length │  CMD   │       Data        │ CRC16  │
│  1 B   │  1 B   │     (0-255) B     │  2 B   │
└────────┴────────┴───────────────────┴────────┘

Length = Data字节数 + 3 (1 CMD + 2 CRC)
```

### MCU → 上位机（应答帧）

与命令帧结构完全相同。

### 字段说明

| 字段 | 字节数 | 说明 |
|:---|:---|:---|
| Length | 1 | 帧总长度-1。等于 CMD(1) + Data(N) + CRC(2) 的字节数，范围 3~255 |
| CMD | 1 | 命令码 |
| Data | 0~252 | 数据载荷，长度 = Length - 3 |
| CRC16 | 2 | CRC-16/MODBUS，低位在前。校验范围：CMD + Data |

### CRC 算法

- CRC-16/MODBUS
- 多项式：0x8005
- 初始值：0xFFFF
- 查表法实现（查表已附于文末）
- 校验范围：CMD 字段开始到 Data 字段结束（不含 Length，不含 CRC 自身）

### 帧识别

接收端持续读取串口字节，按以下规则找到帧边界：

```
1. 读 1 字节 → Length
2. 读 Length 字节 → CMD + Data + CRC
3. 验证 CRC
4. CRC 正确 → 帧有效，处理
5. CRC 错误 → 丢弃，回到步骤 1
```

等待帧间超时由上位机处理（建议 3 秒），MCU 帧间无需超时。

## 命令定义

### 命令码表

| 码 | 名称 | 方向 | Data 内容 |
|:---|:---|:---|:---|
| 0x01 | PING | PC→MCU | 无 |
| 0x02 | PONG | MCU→PC | 1B status + 2B version |
| 0x03 | ERASE | PC→MCU | 无 |
| 0x04 | ERASE_ACK | MCU→PC | 1B status (0=OK) |
| 0x05 | WRITE | PC→MCU | 4B addr + 256B data |
| 0x06 | WRITE_ACK | MCU→PC | 1B status + 4B addr |
| 0x07 | VERIFY | PC→MCU | 4B size + 2B crc16 |
| 0x08 | VERIFY_ACK | MCU→PC | 1B status (0=OK) |
| 0x09 | RESET | PC→MCU | 无 |

### 各命令详情

---

#### PING (0x01) — 握手

```
PC → MCU:  [0x03] [0x01] [CRC_LO] [CRC_HI]
MCU → PC:  [0x05] [0x02] [0x00] [VER_LO] [VER_HI] [CRC_LO] [CRC_HI]
```

- status=0 表示就绪
- version=0x0100 表示 v1.0

---

#### ERASE (0x03) — 擦除 APP 区

```
PC → MCU:  [0x03] [0x03] [CRC_LO] [CRC_HI]
MCU → PC:  [0x04] [0x04] [0x00] [CRC_LO] [CRC_HI]
```

- MCU 收到后擦除 Sector 1~5 (0x08004000 ~ 0x0803FFFF)
- 擦除完成后返回 status=0

---

#### WRITE (0x05) — 写入数据块

```
PC → MCU:  [0xFE] [0x05] [ADDR_4B] [DATA_256B] [CRC_LO] [CRC_HI]
                                ← Data 256B →
MCU → PC:  [0x07] [0x06] [STATUS] [ADDR_4B] [CRC_LO] [CRC_HI]
```

- Length = 0xFE ≈ 254 (1 CMD + 4 addr + 256 data + 2 CRC = 263 → 超出 255)
- ⚠ Length 字段只有 1 字节，最大 255。4+256+2=262 > 255
- **修正**：写操作分两帧——
  - 第一帧：4B 地址
  - 第二帧：256B 数据

**修正后 WRITE 协议：**

```
第一步 — 发送地址:
PC → MCU:  [0x07] [0x05] [ADDR_4B] [CRC_LO] [CRC_HI]
MCU → PC:  [0x04] [0x06] [0x00] [CRC_LO] [CRC_HI]   (ACK)

第二步 — 发送数据:
PC → MCU:  [0x04] [0x0A] [DATA_256B] [... 256B ...] [CRC_LO] [CRC_HI]
                                          ← 共 256B →
MCU → PC:  [0x04] [0x06] [0x00] [CRC_LO] [CRC_HI]   (ACK)
```

- 0x0A 命令用于传输 256 字节数据块
- MCU 收到 DATA 帧后，写入此前收到的 ADDR 地址
- 写入成功后返回 status=0，ADDR 自动向后偏移 256

命令码更新：

| 码 | 名称 | 方向 | Data 内容 |
|:---|:---|:---|:---|
| 0x05 | WRITE_ADDR | PC→MCU | 4B addr |
| 0x06 | ACK | MCU→PC | 1B status (0=OK) |
| 0x0A | WRITE_DATA | PC→MCU | 256B data |

---

#### VERIFY (0x07) — 校验固件

```
PC → MCU:  [0x09] [0x07] [SIZE_4B] [CRC_2B] [CRC_LO] [CRC_HI]
MCU → PC:  [0x04] [0x08] [0x00] [CRC_LO] [CRC_HI]
```

- SIZE：固件字节数
- CRC：上位机计算的全固件 CRC-16/MODBUS
- MCU 从 0x08004000 起读 SIZE 字节，计算 CRC，与上位机给的比较
- 匹配返回 status=0

---

#### RESET (0x09) — 软复位

```
PC → MCU:  [0x03] [0x09] [CRC_LO] [CRC_HI]
```

- MCU 发送 ACK 后立即执行 NVIC_SystemReset()
- 无需等待回复

## 交互流程

```
上位机                               MCU (Bootloader)
  │                                       │
  ├─ PING ────────────────────────────────→│
  │                          ←── PONG ────┤
  │                                       │
  ├─ ERASE ───────────────────────────────→│
  │                        ←── ERASE_ACK ─┤
  │                                       │
  ├─ WRITE_ADDR(addr=0x08004000) ────────→│
  │                               ←── ACK ┤
  ├─ WRITE_DATA(block[0..255]) ──────────→│
  │                               ←── ACK ┤
  ├─ WRITE_ADDR(addr=0x08004100) ────────→│
  │                               ←── ACK ┤
  ├─ WRITE_DATA(block[256..511]) ────────→│
  │                               ←── ACK ┤
  │            ... (重复 N 次) ...          │
  │                                       │
  ├─ VERIFY(size, crc) ─────────────────→│
  │                      ←── VERIFY_ACK ─┤
  │                                       │
  ├─ RESET ──────────────────────────────→│
  │                                       │
  └─ (MCU 重启，Bootloader 检测 APP 有效 → 跳转 APP)
```

## 错误处理

- MCU 侧：任何无效 Length/CMD/CRC → 静默丢弃，不回复
- PC 侧：发送命令后等待应答，3 秒超时 → 提示用户并重试（最多 3 次）
- MCU 收到 RESET 前的任何时刻，上位机可重发 PING 重新开始

## 附录：CRC-16/MODBUS 查表

```c
static const uint16_t crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t crc16_calc(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    for (uint16_t i = 0u; i < len; i++) {
        crc = (uint16_t)((crc >> 8u) ^ crc16_table[(crc ^ data[i]) & 0xFFu]);
    }
    return crc;
}
```

## 附录：命令码总结

| 码 | 名称 | 方向 | Length | Data | 说明 |
|:---|:---|:---|:---|:---|:---|
| 0x01 | PING | → | 3 | - | 握手 |
| 0x02 | PONG | ← | 5 | 1B status, 2B ver | 握手回复 |
| 0x03 | ERASE | → | 3 | - | 擦除 APP |
| 0x04 | ERASE_ACK | ← | 4 | 1B status | 擦除完成 |
| 0x05 | WRITE_ADDR | → | 7 | 4B addr | 设置写入地址 |
| 0x06 | ACK | ← | 4 | 1B status | 操作确认 |
| 0x07 | VERIFY | → | 9 | 4B size, 2B crc | 校验请求 |
| 0x08 | VERIFY_ACK | ← | 4 | 1B status | 校验完成 |
| 0x09 | RESET | → | 3 | - | 软复位 |
| 0x0A | WRITE_DATA | → | 4 (len=0x04)→ | 256B data | 写入数据块 |

> **注意**：WRITE_DATA 的 Length 字段固定为 0x04（与 Length=数据字节数+3 的公式一致：CMD(1) + Data(256) + CRC(2) = 259 → 超出 255，所以 DATA 帧单独处理。实际实现中 Length 使用 uint8_t，0x04 表示接收端期待 256B data + 1B CMD + 2B CRC 已被计入，直接收 256B 数据即可）
>
> **⚠ 修正**：Length=1 字节，最大 255。Data(256) + CMD(1) + CRC(2) = 259 > 255。
> 改为：WRITE_DATA 帧不设 Length，固定帧格式：
> ```
> [CMD=0x0A][DATA=256B][CRC=2B]
> ```
> 接收端一旦识别 CMD=0x0A，固定读取 256 字节 Data + 2 字节 CRC。

**最终命令码**：

| 码 | 名称 | 方向 | 帧格式 | 说明 |
|:---|:---|:---|:---|:---|
| 0x01 | PING | → | [Len=3][0x01][CRC2] | 握手 |
| 0x02 | PONG | ← | [Len=5][0x02][status][ver_lo][ver_hi][CRC2] | 握手回复 |
| 0x03 | ERASE | → | [Len=3][0x03][CRC2] | 擦除 APP |
| 0x04 | ERASE_ACK | ← | [Len=4][0x04][status][CRC2] | 擦除完成 |
| 0x05 | WRITE_ADDR | → | [Len=7][0x05][addr_4B][CRC2] | 设置地址 |
| 0x06 | ACK | ← | [Len=4][0x06][status][CRC2] | 确认 |
| 0x07 | VERIFY | → | [Len=9][0x07][size_4B][crc_2B][CRC2] | 校验 |
| 0x08 | VERIFY_ACK | ← | [Len=4][0x08][status][CRC2] | 校验完成 |
| 0x09 | RESET | → | [Len=3][0x09][CRC2] | 软复位 |
| 0x0A | WRITE_DATA | → | [0x0A][data_256B][CRC2] | 写入数据（固定长度，无Len字段） |

**特殊规则**：
- WRITE_DATA (0x0A) 无 Length 字段。接收端检测到 CMD=0x0A 后直接读取 258 字节（256 Data + 2 CRC）。
- 其他所有帧：先读 1 字节 Length → 读 Length 字节（CMD + Data + CRC）→ CRC 校验。
