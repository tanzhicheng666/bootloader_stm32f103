import serial
import struct
import time

PORT = 'COM10'
BAUD = 115200
PAYLOAD_MAX = 512

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

def stm32_hw_crc32(data: bytes) -> int:
    """已验证：refin=False, refout=False, endian=little"""
    # 🌟 核心修改：不足 4 字节的尾部按照 STM32 写入驱动的逻辑，补 0x00 
    padded = data + b'\x00' * ((4 - len(data) % 4) % 4)
    
    crc = 0xFFFFFFFF
    poly = 0x04C11DB7
    for i in range(0, len(padded), 4):
        word = struct.unpack('<I', padded[i:i+4])[0]
        crc ^= word
        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc

ser = serial.Serial(PORT, BAUD, timeout=2)
time.sleep(0.1)

# 读取测试固件
with open('APP_A2.bin', 'rb') as f:
    firmware = f.read()

firmware_size = len(firmware)
total_pkts = (firmware_size + PAYLOAD_MAX - 1) // PAYLOAD_MAX
firmware_crc = stm32_hw_crc32(firmware)

print(f"固件大小: {firmware_size}, 总包数: {total_pkts}, CRC32: 0x{firmware_crc:08X}")

# 发握手帧
raw_hs = struct.pack('>BBIHI', 0xA5, 0x10, firmware_size, total_pkts, firmware_crc)
crc_hs = crc16_modbus(raw_hs)
frame_hs = raw_hs + struct.pack('>H', crc_hs)
ser.write(frame_hs)
time.sleep(0.3)

resp = ser.read(6)
if not (len(resp) == 6 and resp[0] == 0xA6 and resp[1] == 0x11):
    print(f"❌ 握手失败: {resp.hex()}")
    ser.close()
    exit()
print("✅ 握手成功，开始发送数据包")

# 逐包发送
for seq in range(total_pkts):
    start = seq * PAYLOAD_MAX
    end = min(start + PAYLOAD_MAX, firmware_size)
    payload = firmware[start:end]
    length = len(payload)
    
    crc_buf = struct.pack('>HH', seq, length) + payload
    crc_data = crc16_modbus(crc_buf)
    frame = struct.pack('>BBHH', 0xA5, 0x20, seq, length) + payload + struct.pack('>H', crc_data)
    
    ser.write(frame)
    time.sleep(0.05)  # 给STM32写Flash留时间
    
    resp = ser.read(6)
    if len(resp) == 6 and resp[1] == 0x80:
        print(f"包{seq}/{total_pkts-1} ✅ ACK")
    else:
        print(f"包{seq}/{total_pkts-1} ❌ 失败: {resp.hex() if resp else '无响应'}")
        break
else:
    print("🎉 全部发送完成")

print("🎉 全部发送完成，正在等待 STM32 校验反馈...")

# 给 STM32 预留足够的运算与打印时间
time.sleep(0.5)

# 只要串口中还有残留的数据流，一律按文本行安全读取
while ser.in_waiting > 0:
    line = ser.readline()
    try:
        # 解码成标准 ASCII 字符串输出
        line_str = line.decode('ascii', errors='ignore').strip()
        if line_str:
            print(f"STM32 提示: {line_str}")
    except Exception:
        pass

ser.close()