# 回发代码测试
import serial
import time

# 改成你自己的串口号
ser = serial.Serial('COM10', 115200, timeout=2)
time.sleep(0.1)

# 发一个字节 0xAA
ser.write(bytes([0xAA]))
print("已发送 0xAA")

# 等待回复
time.sleep(0.5)
if ser.in_waiting > 0:
    data = ser.read(ser.in_waiting)
    print("收到:", data.hex())
else:
    print("没有收到回复")

ser.close()