/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_communicate.c
 *  Description: Application layer - bootloader communication protocol
 *               Frame parser with CRC-16/MODBUS validation,
 *               command dispatch to registered handlers.
 *
 *  Protocol: [Length 1B][CMD 1B][Data N B][CRC16 2B]
 *            Length = N + 3 (CMD + CRC)
 *            WRITE_DATA (0x0A): [CMD 1B][Data 256B][CRC16 2B] (no Length)
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_communicate.h"
#include "bsp_uart.h"
#include "bsp_gpio.h"

/*==============================================================================
 * CRC-16/MODBUS Table
 * Polynomial: 0x8005, Initial: 0xFFFF, LSB-first
 *============================================================================*/
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

/*==============================================================================
 * Receive State Machine States
 *============================================================================*/
typedef enum {
    COMM_RX_STATE_IDLE        = 0,  /* waiting for Length byte */
    COMM_RX_STATE_RECV_FRAME  = 1,  /* receiving standard frame */
    COMM_RX_STATE_RECV_WRITE  = 2,  /* receiving WRITE_DATA frame */
} comm_rx_state_t;

/*==============================================================================
 * Local Variables
 *============================================================================*/
static comm_rx_state_t g_rx_state = COMM_RX_STATE_IDLE;
static uint8_t  g_rx_buf[APP_COMM_WRITE_DATA_FRAME_LEN];  /* max frame: WRITE_DATA */
static uint16_t g_rx_index = 0;
static uint16_t g_rx_target = 0;

/* WRITE_DATA CRC: covers CMD(0x0A) + data(256B) */
static uint8_t g_write_data_cmd_byte = 0;

static app_comm_handler_t g_handler_table[256];

/*==============================================================================
 * CRC Functions
 *============================================================================*/

/*
 *  Function: static uint16_t app_comm_crc16(const uint8_t *data, uint16_t len)
 *  Description: Calculate CRC-16/MODBUS over the given data.
*/
static uint16_t app_comm_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;
    uint16_t i;

    for (i = 0; i < len; i++)
    {
        crc = (uint16_t)((crc >> 8u) ^ crc16_table[(crc ^ data[i]) & 0xFFu]);
    }
    return crc;
}

/*==============================================================================
 * Send Functions
 *============================================================================*/

/*
 *  Function: void app_comm_send_frame(uint8_t cmd, uint8_t *data, uint16_t len)
 *  Input:  cmd  - command code
 *          data - payload buffer (may be NULL if len == 0)
 *          len  - payload length in bytes
 *  Output: none
 *  Return: none
 *  Description: Build and send a standard protocol frame:
 *               [Length][CMD][Data...][CRC_LO][CRC_HI]
 *               Length = len + 3 (CMD + CRC)
 *               CRC covers CMD + Data.
*/
void app_comm_send_frame(uint8_t cmd, uint8_t *data, uint16_t len)
{
    uint8_t  frame[APP_COMM_FRAME_LEN_MAX + 1];  /* Length byte + max 255 */
    uint8_t  length;
    uint16_t crc;
    uint16_t i;

    length = (uint8_t)(len + 3U);  /* CMD + Data + CRC */

    frame[0] = length;
    frame[1] = cmd;

    for (i = 0; i < len; i++)
    {
        frame[2 + i] = data[i];
    }

    /* CRC covers CMD + Data */
    crc = app_comm_crc16(&frame[1], (uint16_t)(len + 1U));

    /* CRC little-endian (low byte first) */
    frame[2 + len]     = (uint8_t)(crc & 0xFFu);
    frame[2 + len + 1] = (uint8_t)(crc >> 8u);

    bsp_uart_send(frame, (uint16_t)(length + 1U));
}

/*==============================================================================
 *  Function: void app_comm_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize the communication module.
 *               Clears handler table and resets receive state machine.
 *============================================================================*/
void app_comm_init(void)
{
    uint16_t i;

    for (i = 0; i < 256; i++)
    {
        g_handler_table[i] = (app_comm_handler_t)0;
    }

    g_rx_state  = COMM_RX_STATE_IDLE;
    g_rx_index  = 0;
    g_rx_target = 0;
}

/*==============================================================================
 *  Function: void app_comm_register_handler(uint8_t cmd,
 *                                           app_comm_handler_t handler)
 *  Input:  cmd     - command code to handle
 *          handler - callback function
 *  Output: none
 *  Return: none
 *  Description: Register a handler for a specific command code.
 *               When a valid frame with this CMD is received,
 *               handler(data, len) is called.
 *============================================================================*/
void app_comm_register_handler(uint8_t cmd, app_comm_handler_t handler)
{
    g_handler_table[cmd] = handler;
}

/*==============================================================================
 *  Function: static void app_comm_dispatch(uint8_t cmd, uint8_t *data,
 *                                          uint16_t len)
 *  Description: Call the registered handler for the given command.
 *============================================================================*/
static void app_comm_dispatch(uint8_t cmd, uint8_t *data, uint16_t len)
{
    if (g_handler_table[cmd] != (app_comm_handler_t)0)
    {
        g_handler_table[cmd](data, len);
    }
}

/*==============================================================================
 *  Function: void app_comm_process_byte(uint8_t byte)
 *  Input:  byte - one UART received byte
 *  Output: none
 *  Return: none
 *  Description: Feed a single byte into the frame parser state machine.
 *
 *  State machine:
 *    IDLE: If byte == 0x0A → WRITE_DATA, target=258.
 *          Else → Length. If valid (3-255), RECV_FRAME, target=Length.
 *
 *    RECV_FRAME: collect target bytes → verify CRC → dispatch → IDLE.
 *
 *    RECV_WRITE: collect 258 bytes (data+CRC) → verify CRC → dispatch → IDLE.
 *============================================================================*/
void app_comm_process_byte(uint8_t byte)
{
    uint16_t crc_calc;
    uint16_t crc_recv;
    uint8_t  cmd;
    uint16_t data_len;

    switch (g_rx_state)
    {
    case COMM_RX_STATE_IDLE:
        if (byte == APP_COMM_CMD_WRITE_DATA)
        {
            bsp_uart_send_byte(0xA0);  /* IDLE→WRITE_DATA */
            g_rx_state  = COMM_RX_STATE_RECV_WRITE;
            g_rx_target = APP_COMM_WRITE_DATA_FRAME_LEN;
            g_rx_index  = 0;
            g_write_data_cmd_byte = byte;
        }
        else
        {
            if (byte < APP_COMM_FRAME_LEN_MIN || byte > APP_COMM_FRAME_LEN_MAX)
            {
                bsp_uart_send_byte(0xA1);  /* invalid Length */
            }
            else
            {
                bsp_uart_send_byte(0xA2);  /* IDLE→RECV_FRAME */
                g_rx_state  = COMM_RX_STATE_RECV_FRAME;
                g_rx_target = byte;
                g_rx_index  = 0;
            }
        }
        break;

    case COMM_RX_STATE_RECV_FRAME:
        g_rx_buf[g_rx_index++] = byte;

        if (g_rx_index >= g_rx_target)
        {
            cmd      = g_rx_buf[0];
            data_len = g_rx_target - 3U;

            crc_calc = app_comm_crc16(g_rx_buf, data_len + 1U);
            crc_recv = (uint16_t)g_rx_buf[g_rx_target - 2] |
                      ((uint16_t)g_rx_buf[g_rx_target - 1] << 8);

            if (crc_calc == crc_recv)
            {
                bsp_uart_send_byte(0xA3);  /* CRC OK */
                app_comm_dispatch(cmd, &g_rx_buf[1], data_len);
            }
            else
            {
                bsp_uart_send_byte(0xA4);  /* CRC FAIL */
            }

            g_rx_state = COMM_RX_STATE_IDLE;
            g_rx_index = 0;
        }
        break;

    case COMM_RX_STATE_RECV_WRITE:
        g_rx_buf[g_rx_index++] = byte;

        if (g_rx_index >= g_rx_target)
        {
            data_len = APP_COMM_WRITE_DATA_SIZE;

            crc_calc = app_comm_crc16(&g_write_data_cmd_byte, 1);
            {
                uint16_t i;
                for (i = 0; i < data_len; i++)
                {
                    crc_calc = (uint16_t)((crc_calc >> 8u) ^
                        crc16_table[(crc_calc ^ g_rx_buf[i]) & 0xFFu]);
                }
            }

            crc_recv = (uint16_t)g_rx_buf[data_len] |
                      ((uint16_t)g_rx_buf[data_len + 1] << 8);

            if (crc_calc == crc_recv)
            {
                app_comm_dispatch(APP_COMM_CMD_WRITE_DATA,
                                  g_rx_buf, data_len);
            }

            g_rx_state = COMM_RX_STATE_IDLE;
            g_rx_index = 0;
        }
        break;

    default:
        g_rx_state = COMM_RX_STATE_IDLE;
        g_rx_index = 0;
        break;
    }
}

/*==============================================================================
 *  Function: void app_comm_process(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Process all available UART bytes through the frame parser.
 *               Call periodically from a task.
 *============================================================================*/
void app_comm_process(void)
{
    while (bsp_uart_available())
    {
        app_comm_process_byte(bsp_uart_read());
    }
}
