# Bootloader 通信协议规范 v1.0

> 创建日期: 2026-06-22

---

## 1. 物理层

| 参数 | 值 |
|------|-----|
| 接口 | USART2 (PA2=TX, PA3=RX) |
| 波特率 | 115200 bps |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | 无 |
| 流控 | 无 |

---

## 2. 帧格式

### 2.0 帧头帧尾

| 字段 | 值 | 说明 |
|------|----|------|
| SOF (Start of Frame) | `0xAA` | 帧起始标记 |
| EOF (End of Frame) | `0x55` | 帧结束标记 |

CRC 覆盖 CMD + Data，不覆盖 SOF/EOF/Length。

### 2.1 标准帧

```
┌──────┬──────────┬──────────┬─────────────┬───────────────┬──────┐
│ SOF  │  Length  │   CMD    │    Data      │    CRC-16     │ EOF  │
│ 0xAA │   1 B    │   1 B    │   N bytes    │    2 bytes     │ 0x55 │
└──────┴──────────┴──────────┴─────────────┴───────────────┴──────┘
                        │◄──────── CRC 覆盖 ────────►│
```

| 字段 | 字节 | 说明 |
|------|------|------|
| SOF | 1 | `0xAA`，帧头 |
| Length | 1 | 帧内长度 = CMD(1) + Data(N) + CRC(2) = N + 3 |
| CMD | 1 | 命令码 |
| Data | N | 负载数据 |
| CRC | 2 | CRC-16/MODBUS（覆盖 CMD+Data），低字节在前 |
| EOF | 1 | `0x55`，帧尾 |

**约束：**
- Length 合法范围：**3 ~ 255**
- 总帧长 = Length + 3 字节（SOF + Length字节 + EOF）= 6 ~ 258 字节

### 2.2 WRITE_DATA 帧（特殊）

```
┌──────┬──────────┬──────────────────────┬───────────────┬──────┐
│ SOF  │   CMD    │        Data          │    CRC-16     │ EOF  │
│ 0xAA │   0x0A   │      256 bytes       │    2 bytes     │ 0x55 │
│ 1 B  │   1 B    │       256 B          │     2 B        │ 1 B  │
└──────┴──────────┴──────────────────────┴───────────────┴──────┘
                 │◄──── CRC 覆盖 ───────►│
```

**特点：**
- 无 Length 字节，CMD 固定为 0x0A
- 接收端先收 SOF(0xAA)，然后收 CMD，若为 0x0A → 进入 WRITE_DATA 模式，等待 258 字节（256 data + 2 CRC），最后收 EOF(0x55) 校验
- CRC 覆盖 CMD(0x0A) + Data(256B)
- 总帧长固定 = 1 + 1 + 256 + 2 + 1 = **261 字节**

**为什么特殊：** 标准帧的 Length 字段是 uint8_t（最大 255），装不下 257 字节的有效载荷（1 CMD + 256 data + 2 CRC = 259）。所以去掉 Length，用 CMD 值来识别。

---

## 3. CRC 规格

| 参数 | 值 |
|------|-----|
| 算法 | CRC-16/MODBUS |
| 多项式 | 0x8005 |
| 初始值 | 0xFFFF |
| 输入反转 | 是（LSB first） |
| 输出反转 | 是 |
| 结果异或 | 0x0000 |
| 字节序 | 低字节在前（little-endian） |

---

## 4. 命令表

### 4.1 主机 → MCU (请求)

| 命令 | 代码 | Data | 说明 |
|------|------|------|------|
| PING | 0x01 | 无 | 握手检测，确认 bootloader 在线 |
| ERASE | 0x03 | 无 | 擦除参数页（页 4） |
| WRITE_ADDR | 0x05 | addr[4] LE | 设置 Flash 写入起始地址 |
| WRITE_DATA | 0x0A | data[256] | 256 字节数据块（特殊帧） |
| VERIFY | 0x07 | addr[4]+len[4]+crc[4] | 校验固件 CRC |
| RESET | 0x09 | 无 | 软件复位 MCU |

### 4.2 MCU → 主机 (应答)

| 命令 | 代码 | Data | 说明 |
|------|------|------|------|
| PONG | 0x02 | version[2] | PING 应答，低字节为主版本号 |
| ERASE_ACK | 0x04 | status[1] | 擦除结果 |
| ACK | 0x06 | status[1] | 通用应答 |
| VERIFY_ACK | 0x08 | status[1] | 校验结果 |

### 4.3 状态码

| 代码 | 含义 |
|------|------|
| 0x00 | 成功 (STATUS_OK) |
| 0x01 | 失败 (STATUS_ERR) |

---

## 5. 命令详解

### 5.1 PING → PONG

```
PC → MCU:   AA 03 01 7E 80 55
            │  │  │  └──────└─ CRC + EOF
            │  │  └─────────── CMD = PING
            │  └────────────── Length = 3 (CMD+CRC)
            └───────────────── SOF = 0xAA

MCU → PC:   AA 05 02 00 01 00 C0 5C 55
            │  │  │  └─────└── version = 0x0001 (v1.0)
            │  │  └─────────── CMD = PONG
            │  └────────────── Length = 5 (CMD+2+CRC)
            └───────────────── SOF
```

### 5.2 ERASE → ERASE_ACK

```
PC → MCU:   AA 03 03 F5 5D 55
            │  │  │  └───── CRC + EOF
            │  │  └──────── CMD = ERASE
            │  └─────────── Length = 3
            └────────────── SOF

MCU → PC:   AA 04 04 00 7B 2A 55
            │  │  │  │  └──── status=OK, CRC, EOF
            │  │  │  └─────── CMD = ERASE_ACK
            │  └───────────── Length = 4
            └──────────────── SOF
```

MCU 收到 ERASE 后擦除参数页（页 4），返回结果。

### 5.3 WRITE_ADDR → ACK

```
PC → MCU:   07 05 00 28 00 08 XX XX
            │  │  └─────────└─ addr = 0x08002800 (APP 起始地址)
            │  └────────────── CMD = WRITE_ADDR
            └───────────────── Length = 7 (CMD+4+CRC)

MCU → PC:   04 06 00 XX XX
            │  │  │
            │  │  └── status
            │  └───── CMD = ACK
            └──────── Length = 4
```

MCU 存储此地址为写入起始地址。地址必须在 APP 区范围内（0x08002800 ~ 0x0803FFFF），否则返回 STATUS_ERR。

### 5.4 WRITE_DATA → ACK

```
PC → MCU:   0A  XX XX ... (256 bytes) ...  XX XX
            │   └──────────────────┘          └── CRC16 (2B, 覆盖 CMD+Data)
            └── CMD = WRITE_DATA (0x0A)
```

每成功写入 256 字节 → 返回 ACK(OK)；写入失败 → ACK(ERR)。写完后地址自动 +256，等待下一个 WRITE_DATA 或 VERIFY。

### 5.5 VERIFY → VERIFY_ACK

```
PC → MCU:   0F 07 00 28 00 08 00 00 04 00 XX XX XX XX XX XX
            │  │  └───────────────┘ └───────────────┘ └──────┘
            │  │   addr = 0x08002800  len = 0x00040000  expected_crc
            │  └── CMD = VERIFY
            └───── Length = 15

MCU → PC:   04 08 00 XX XX
            │  │  │
            │  │  └── status (OK/ERR)
            │  └───── CMD = VERIFY_ACK
            └──────── Length = 4
```

MCU 从 addr 开始读 len 个字节，计算 CRC-16/MODBUS，与 expected_crc 比对。一致 → OK，不一致 → ERR。

### 5.6 RESET → ACK → 复位

```
PC → MCU:   03 09 C5 6D
            │  │  └── CRC
            │  └───── CMD = RESET
            └──────── Length = 3

MCU → PC:   04 06 00 XX XX     ← ACK(OK) 帧
            │
            └── 延时 10ms → NVIC_SystemReset()
```

---

## 6. 接收端状态机

```
                              ┌─────────┐
                              │  IDLE   │
                              │ (等SOF) │
                              └────┬────┘
                                   │ byte == 0xAA?
                              ┌────┴────┐
                              │ 否:再等  │
                              │ 是:收CMD│
                              └────┬────┘
                                   ▼
                              byte == 0x0A?
                         ┌─────┴─────┐
                         │ 是         │ 否 (标准帧)
                         ▼            ▼
                  ┌──────────┐  ┌──────────┐
                  │RECV_WRITE│  │RECV_FRAME│
                  │收258字节  │  │取首字节  │
                  │+ 等EOF   │  │→ Length  │
                  │(共259B)   │  │收Length  │
                  └────┬─────┘  │个字节    │
                       │收完    └────┬─────┘
                       ▼             │收完
                  ┌──────────┐       ▼
                  │ 等EOF    │  ┌──────────┐
                  │ byte==   │  │ 等EOF    │
                  │  0x55?   │  │ byte==   │
                  └──┬───┬───┘  │  0x55?   │
              错→IDLE│   │对    └──┬───┬───┘
                     │   ▼   错→IDLE│   │对
                     │ ┌─────┐      │   ▼
                     │ │CRC验│      │ ┌─────┐
                     │ │CMD+ │      │ │CRC验│
                     │ │Data │      │ │CMD+ │
                     │ └┬───┬┘      │ │Data │
                     │  │对 │       │ └┬───┬┘
                     │  ▼  │错      │  │对 │错
                     │disp │→IDLE   │  ▼   │→IDLE
                     │→IDLE│        │disp  │
                     └─────┘        │→IDLE │
                                    └──────┘
```

**关键规则：**
- 收到 SOF(0xAA) 前，丢弃所有字节
- 收到 EOF(0x55) 后才校验 CRC——EOF 在则帧完整
- EOF 不匹配 → 静默丢弃帧，回到 IDLE
- CRC 错误 → 静默丢弃帧，不发送错误响应
- SOF 出现在帧中间 → 重新开始（自动同步恢复）
- Length < 3 或 > 255 → 无效帧，回到 IDLE

---

## 7. 升级流程

```
PC                              MCU
 │                               │
 ├─ PING ──────────────────────→│ Step 1: 握手
 │←────────────── PONG + ver ──┤
 │  (确认 bootloader 在线)       │
 │                               │
 ├─ ERASE ─────────────────────→│ Step 2: 擦除参数区
 │←────────────── ERASE_ACK ────┤
 │  (清除旧标志，准备写入)        │
 │                               │
 ├─ WRITE_ADDR(APP起始) ───────→│ Step 3: 设置首地址
 │←────────────── ACK ──────────┤
 │                               │
 ├─ WRITE_DATA(256B) ──────────→│ Step 4: 逐块写入
 │←────────────── ACK ──────────┤
 │         ... 重复 N 次 ...     │  (每块 256B，地址自动递增)
 │                               │
 ├─ VERIFY(addr,len,crc) ──────→│ Step 5: 整固件校验
 │←────────────── VERIFY_ACK ───┤
 │                               │
 ├─ RESET ─────────────────────→│ Step 6: 复位
 │                               │ → 上电 → 检测标志 → 跳转 APP
```

---

## 8. 错误处理规则

| 情况 | MCU 行为 |
|------|---------|
| CRC 校验失败 | 静默丢弃，不响应 |
| Length 非法 (<3 或 >255) | 静默丢弃，不响应 |
| 写入地址不在 APP 区 | 返回 ACK(ERR) |
| Flash 写入失败 | 返回 ACK(ERR) |
| VERIFY CRC 不匹配 | 返回 VERIFY_ACK(ERR) |
| UART overrun/noise | 清标志，重启接收 |

---

## 9. 待讨论

1. **VERIFY 的 CRC 粒度**：用 CRC-16/MODBUS 还是 CRC-32？CRC-16 足够检测传输错误，但 CRC-32 更安全。你的意见？
2. **超时机制**：如果 PC 发送了 WRITE_DATA 但 MCU 没收到（无 ACK），PC 的超时重发机制如何设计？还是放在上位机层？
3. **写入地址范围校验**：APP 起始地址硬编码为 0x08002800，还是由 WRITE_ADDR 动态指定？前者更安全。
4. **单帧最大长度**：当前标准帧最大 256 字节（Length = uint8_t），够用吗？
