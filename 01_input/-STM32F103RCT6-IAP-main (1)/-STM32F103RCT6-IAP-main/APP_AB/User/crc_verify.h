#ifndef CRC_VERIFY_H
#define CRC_VERIFY_H

#include <stdint.h>
#include <stdbool.h>

uint16_t APP_Compute_CRC16(const uint8_t *data, uint16_t length);
bool APP_CRC16_Verify(const uint8_t* data,uint16_t len,uint16_t except);
bool APP_Ex_CRC16_Verify(const uint8_t *payload, uint16_t len, uint16_t seq, uint16_t received_crc);
//flash写入后的CRC校验
bool APP_Firmware_CRC32_Verify(uint32_t flash_addr, uint32_t length_bytes, uint32_t expected_crc32);

#endif

