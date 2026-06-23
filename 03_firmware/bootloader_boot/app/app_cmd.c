/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_cmd.c
 *  Description: Bootloader command handlers
 *               PING / ERASE / WRITE_ADDR / WRITE_DATA / VERIFY / RESET
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.23            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_cmd.h"
#include "app_comm.h"
#include "bsp_gpio.h"
#include "bsp_flash.h"
#include "bsp_tick.h"
#include "algo_crc.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 *  Shared: flash write address, auto-incremented after each WRITE_DATA block
 *============================================================================*/
uint32_t g_write_addr;

/*==============================================================================
 *  PING — respond with PONG + protocol version
 *============================================================================*/
void comm_handler_ping(uint8_t *data, uint16_t len)
{
    uint8_t resp[3];

    (void)data;
    (void)len;

    resp[0] = APP_COMM_STATUS_OK;
    resp[1] = (uint8_t)(APP_COMM_VERSION & 0xFFu);
    resp[2] = (uint8_t)(APP_COMM_VERSION >> 8u);

    app_comm_send_frame(APP_COMM_CMD_PONG, resp, 3);
    bsp_led_debug_toggle();
}

/*==============================================================================
 *  ERASE — erase parameter page (clears upgrade flag) + APP area
 *============================================================================*/
void comm_handler_erase(uint8_t *data, uint16_t len)
{
    uint8_t status;

    (void)data;
    (void)len;

    if ((bsp_flash_erase_page(BSP_FLASH_PARAM_START) == BSP_FLASH_OK) &&
        (bsp_flash_erase_range(BSP_FLASH_APP_START,
            BSP_FLASH_BASE + BSP_FLASH_SIZE - 1U) == BSP_FLASH_OK))
    {
        status = APP_COMM_STATUS_OK;
    }
    else
    {
        status = APP_COMM_STATUS_ERR;
    }

    app_comm_send_frame(APP_COMM_CMD_ERASE_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*==============================================================================
 *  WRITE_ADDR — store 4-byte LE address, validate range, return ACK
 *============================================================================*/
void comm_handler_write_addr(uint8_t *data, uint16_t len)
{
    uint8_t status;

    (void)len;

    g_write_addr = (uint32_t)data[0]
                 | ((uint32_t)data[1] << 8)
                 | ((uint32_t)data[2] << 16)
                 | ((uint32_t)data[3] << 24);

    if (g_write_addr >= BSP_FLASH_PARAM_START &&
        g_write_addr < (BSP_FLASH_BASE + BSP_FLASH_SIZE))
    {
        status = APP_COMM_STATUS_OK;
    }
    else
    {
        status = APP_COMM_STATUS_ERR;
    }

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*==============================================================================
 *  WRITE_DATA — write 256B block to g_write_addr, auto-increment, return ACK
 *============================================================================*/
void comm_handler_write_data(uint8_t *data, uint16_t len)
{
    uint8_t status;

    (void)len;

    if (bsp_flash_write(g_write_addr, data, APP_COMM_WRITE_DATA_SIZE) == BSP_FLASH_OK)
    {
        g_write_addr += APP_COMM_WRITE_DATA_SIZE;
        status = APP_COMM_STATUS_OK;
    }
    else
    {
        status = APP_COMM_STATUS_ERR;
    }

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*==============================================================================
 *  VERIFY — read flash range, compute CRC-16/MODBUS, compare, return result
 *  Payload: addr[4] + len[4] + expected_crc[2] (10 bytes, all LE)
 *============================================================================*/
void comm_handler_verify(uint8_t *data, uint16_t len)
{
    uint32_t verify_addr;
    uint32_t verify_len;
    uint16_t expected_crc;
    uint16_t calc_crc;
    uint8_t  status;
    uint8_t  buf[256];
    uint32_t offset;
    uint16_t chunk;

    (void)len;

    verify_addr  = (uint32_t)data[0]
                 | ((uint32_t)data[1] << 8)
                 | ((uint32_t)data[2] << 16)
                 | ((uint32_t)data[3] << 24);

    verify_len   = (uint32_t)data[4]
                 | ((uint32_t)data[5] << 8)
                 | ((uint32_t)data[6] << 16)
                 | ((uint32_t)data[7] << 24);

    expected_crc = (uint16_t)data[8]
                 | ((uint16_t)data[9] << 8);

    if (verify_addr < BSP_FLASH_PARAM_START || verify_len == 0)
    {
        status = APP_COMM_STATUS_ERR;
        app_comm_send_frame(APP_COMM_CMD_VERIFY_ACK, &status, 1);
        return;
    }

    /* Compute CRC-16/MODBUS over flash range, chunked at 256B */
    calc_crc = 0xFFFFu;
    offset = 0;

    while (offset < verify_len)
    {
        uint32_t remain = verify_len - offset;
        chunk = (remain > 256U) ? 256U : (uint16_t)remain;

        bsp_flash_read(verify_addr + offset, buf, chunk);

        uint16_t k;
        for (k = 0; k < chunk; k++)
        {
            calc_crc = (uint16_t)((calc_crc >> 8u) ^
                crc16_table[(calc_crc ^ buf[k]) & 0xFFu]);
        }
        offset += chunk;
    }

    status = (calc_crc == expected_crc) ? APP_COMM_STATUS_OK : APP_COMM_STATUS_ERR;
    app_comm_send_frame(APP_COMM_CMD_VERIFY_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*==============================================================================
 *  RESET — send ACK, short delay, then NVIC_SystemReset
 *============================================================================*/
void comm_handler_reset(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);

    /* Allow ACK frame to finish transmitting before reset */
    bsp_tick_delay_ms(10);
    NVIC_SystemReset();
}

// end of file
