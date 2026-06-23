import serial
import struct
import time

# ========== 配置区 ==========
PORT = 'COM10'       # 改成你的串口号
BAUD = 115200
# ============================

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

# 构造握手帧（先用假数据，只测通路）
firmware_size = 0x00001234   # 假固件大小
total_pkts    = 0x000A       # 假包数
firmware_crc  = 0x00000000   # 假CRC32

# 前12字节（不含帧CRC16）
raw = struct.pack('>BBIHI',   # ← HH改成HI
    0xA5,
    0x10,
    firmware_size,
    total_pkts,
    firmware_crc
)

frame_crc = crc16_modbus(raw)
frame = raw + struct.pack('>H', frame_crc)

print(f"发送握手帧({len(frame)}字节): {frame.hex()}")
ser.write(frame)

# 等待ACK（6字节）
time.sleep(0.5)
resp = ser.read(6)
if len(resp) == 6:
    print(f"收到响应: {resp.hex()}")
    sof, typ, seq, crc = struct.unpack('>BBHH', resp)
    print(f"  SOF=0x{sof:02X} TYPE=0x{typ:02X} SEQ={seq} CRC=0x{crc:04X}")
    if sof == 0xA6 and typ == 0x11:
        print("✅ 握手ACK正确")
    else:
        print("❌ 响应内容不对")
else:
    print(f"❌ 没收到完整响应，收到{len(resp)}字节: {resp.hex()}")

ser.close()