import serial
import struct
import time

PORT = 'COM10'
BAUD = 115200

def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

ser = serial.Serial(PORT, BAUD, timeout=2)
time.sleep(0.1)

# ========== 第一步：先发握手帧，让STM32进入WAITDATA状态 ==========
firmware_size = 512        # 就一包，512字节
total_pkts    = 1
firmware_crc  = 0x00000000

raw_hs = struct.pack('>BBIHI', 0xA5, 0x10, firmware_size, total_pkts, firmware_crc)
crc_hs = crc16_modbus(raw_hs)
frame_hs = raw_hs + struct.pack('>H', crc_hs)

print(f"发送握手帧({len(frame_hs)}字节): {frame_hs.hex()}")
ser.write(frame_hs)
time.sleep(0.5)

resp = ser.read(6)
if len(resp) == 6 and resp[0] == 0xA6 and resp[1] == 0x11:
    print("✅ 握手ACK收到，进入数据帧测试")
    time.sleep(0.5)  # 加这一行，给STM32重启DMA的时间
else:
    print(f"❌ 握手失败: {resp.hex()}")
    ser.close()
    exit()

# ========== 第二步：发一包数据帧 ==========
seq     = 0
payload = bytes([0xAA] * 512)   # 假数据，全0xAA
length  = len(payload)

# 数据帧：SOF(1) TYPE(1) SEQ(2) LEN(2) PAYLOAD(N) CRC16(2)

# 修改后：只对 SEQ+LEN+PAYLOAD 算CRC，和STM32对齐
crc_buf = struct.pack('>HH', seq, length) + payload
crc_data = crc16_modbus(crc_buf)

# 帧结构单独拼，CRC不包含帧头
frame_data = struct.pack('>BBHH', 0xA5, 0x20, seq, length) + payload + struct.pack('>H', crc_data)

print(f"发送数据帧({len(frame_data)}字节): seq={seq}")
ser.write(frame_data)
time.sleep(1)

resp2 = ser.read(6)
if len(resp2) == 6:
    sof, typ, rseq, rcrc = struct.unpack('>BBHH', resp2)
    print(f"收到响应: {resp2.hex()}")
    print(f"  SOF=0x{sof:02X} TYPE=0x{typ:02X} SEQ={rseq}")
    if sof == 0xA6 and typ == 0x80 and rseq == seq:
        print("✅ 数据帧ACK正确")
    elif typ == 0x81:
        print("❌ 收到NACK，CRC校验失败")
    else:
        print("❌ 响应内容不对")
else:
    print(f"❌ 没收到响应，收到{len(resp2)}字节: {resp2.hex()}")

ser.close()