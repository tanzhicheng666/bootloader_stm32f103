#include "crc_verify.h"
#include "crc.h"


/**
 * @brief  计算数据的 CRC16
 * @param  data: 待校验的数据缓冲区指针
 * @param  length: 数据长度（字节数）
 * @return uint16_t: 计算出来的 16 位 CRC 结果
 */
uint16_t APP_Compute_CRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF; 

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i]; // 把当前字节异或到 CRC 的低字节中

        for (uint8_t j = 0; j < 8; j++) // 循环处理该字节的 8 个 bit
        {
            if (crc & 0x0001) // 如果最低位是 1
            {
                crc >>= 1;     // 右移一位
                crc ^= 0xA001; // 异或多项式 0xA001 
            }
            else // 如果最低位是 0
            {
                crc >>= 1;     // 直接右移一位
            }
        }
    }

    return crc;
}


/**
 * @brief  校验数据的 CRC16
 * @param  data: 待校验的数据缓冲区指针
 * @param  length: 数据长度（字节数）
* @param	 except:期望的CRC值
 * @return bool: 16 位 CRC 校验结果
 */
bool APP_CRC16_Verify(const uint8_t* data,uint16_t len,uint16_t except)
{
	uint16_t crc=APP_Compute_CRC16(data,len);
	return crc==except?true:false;

}


/**
* @brief  验证接收到的数据包 CRC16 是否正确(此函数内部将uint16_t成功转换成了二进制流)
 * @param  payload: 接收到的真货物缓冲区
 * @param  len: payload的长度
 * @param  seq: 包序号
 * @param  received_crc: 从串口最后 2 个字节读出来的期望 CRC
 * @return bool: true 校验成功，false 校验失败（数据被震歪了）
 */
bool APP_Ex_CRC16_Verify(const uint8_t *payload, uint16_t len, uint16_t seq, uint16_t received_crc)
{
    // 1. 创建一个临时校验缓冲区，把要校验的字段拼起来
    // 为什么要加 4？因为要算上 SEQ(2字节) 和 LEN(2字节)
    uint8_t verify_buf[512 + 4]; 
    uint16_t index = 0;

    // 2. 把 SEQ 塞进去（高字节在前）
    verify_buf[index++] = (uint8_t)(seq >> 8);
    verify_buf[index++] = (uint8_t)(seq & 0xFF);

    // 3. 把 LEN 塞进去（高字节在前）
    verify_buf[index++] = (uint8_t)(len >> 8);
    verify_buf[index++] = (uint8_t)(len & 0xFF);

    // 4. 把真正的 PAYLOAD 货物塞进去
    for(uint16_t i = 0; i < len; i++)
    {
        verify_buf[index++] = payload[i];
    }

    // 5. 现场算一遍整包的本生 CRC
    uint16_t calculated_crc = APP_Compute_CRC16(verify_buf, index);

    // 6. 对比结果
    if (calculated_crc == received_crc)
    {
        return true;  // 封条完好，货物安全！
    }
    else
    {
        return false; // 封条破损，拒绝接收！
    }
}


/**
 * @brief  使用 STM32 硬件 CRC32 校验指定 Flash 区域的固件完整性
 * @param  flash_addr: 固件在 Flash 中的物理起始地址（如 APP_B 的 0x08020800）
 * @param  length_bytes: 固件的实际大小（字节数，通常从握手帧的 firmware_size 获取）
 * @param  expected_crc32: 期望的 CRC32 值（由电脑上位机打包算出，存放在 boot_info 中）
 * @return bool: true 校验通过（固件完好无损），false 校验失败（固件损坏或非法）
 */
bool APP_Firmware_CRC32_Verify(uint32_t flash_addr, uint32_t length_bytes, uint32_t expected_crc32)
{
    // 🛡️ 防线一：基础物理检查
    // 读取固件前 4 个字节（MSP 初始值），检查它是否指向正常的 SRAM 范围内（以 0x20 开头）
//    uint32_t msp_val = *(volatile uint32_t*)flash_addr;
//    if ((msp_val & 0x2FF00000) != 0x20000000)
//    {
//        return false; // 该地址上没有合法固件，直接拦截
//    }

    // 🛡️ 防线二：字节长度转为“字（32位）”长度，并处理 4 字节不对齐的边缘情况
    // 比如：固件实际是 4001 字节，(4001 + 3) / 4 = 1001 个字。后面多出的 3 个字节在 Flash 里本就是 0xFF
    uint32_t length_words = (length_bytes + 3) / 4;

    // 🛡️ 防线三：复位硬件 CRC 寄存器（极其关键！）
    // 防止其他任务（如别的通信协议）残留的 CRC 状态污染了本次固件的计算结果
    __HAL_CRC_DR_RESET(&hcrc);

    // 🚀 调用 HAL 库硬件加速计算
    // 硬件会直接通过主总线全速读取 Flash 对应区间的数据进行物理计算
    uint32_t calculated_crc32 = HAL_CRC_Calculate(&hcrc, (uint32_t*)flash_addr, length_words);

    // ⚖️ 【核心比对】
    if (calculated_crc32 == expected_crc32)
    {
        return true;  // 封条完好，固件百分之百纯净！
    }
    else
    {
        return false; // 校验失败，发生了断电写坏或 Flash 位翻转！
    }
}
