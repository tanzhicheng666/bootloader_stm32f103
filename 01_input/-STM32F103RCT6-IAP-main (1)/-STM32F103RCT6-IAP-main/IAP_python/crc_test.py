import struct

def stm32_hw_crc32(data: bytes) -> int:
    """已验证：refin=False, refout=False, endian=little"""
    padded = data + b'\xFF' * ((4 - len(data) % 4) % 4)
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
test_data = bytes([0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
                    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10])

result = stm32_hw_crc32(test_data)
print(f"Python计算: 0x{result:08X}")

# def crc32_variant(data: bytes, refin: bool, refout: bool, endian: str) -> int:
#     def reverse_bits(x, width):
#         result = 0
#         for _ in range(width):
#             result = (result << 1) | (x & 1)
#             x >>= 1
#         return result
    
#     padded = data + b'\xFF' * ((4 - len(data) % 4) % 4)
#     crc = 0xFFFFFFFF
#     poly = 0x04C11DB7
    
#     for i in range(0, len(padded), 4):
#         fmt = '<I' if endian == 'little' else '>I'
#         word = struct.unpack(fmt, padded[i:i+4])[0]
#         if refin:
#             word = reverse_bits(word, 32)
#         crc ^= word
#         for _ in range(32):
#             if crc & 0x80000000:
#                 crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
#             else:
#                 crc = (crc << 1) & 0xFFFFFFFF
    
#     if refout:
#         crc = reverse_bits(crc, 32)
    
#     return crc

# test_data = bytes([0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
#                     0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10])

# target = 0x550D4818

# for refin in [False, True]:
#     for refout in [False, True]:
#         for endian in ['little', 'big']:
#             result = crc32_variant(test_data, refin, refout, endian)
#             mark = " ✅" if result == target else ""
#             print(f"refin={refin} refout={refout} endian={endian}: 0x{result:08X}{mark}")