/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_comm.c
 *  Description: Application layer — bootloader communication protocol
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_comm.h"
#include "bsp_uart.h"
#include "algo_crc.h"

typedef enum {
    COMM_RX_STATE_IDLE        = 0,
    COMM_RX_STATE_RECV_CMD    = 1,
    COMM_RX_STATE_RECV_FRAME  = 2,
    COMM_RX_STATE_RECV_WRITE  = 3,
    COMM_RX_STATE_WAIT_EOF    = 4,
} en_comm_rx_state_t;

static en_comm_rx_state_t g_rx_state = COMM_RX_STATE_IDLE;
static uint8_t  g_rx_buf[APP_COMM_WRITE_FRAME_LEN + 2];
static uint16_t g_rx_index;
static uint16_t g_rx_target;
static app_comm_handler_t g_handler_table[256];

void app_comm_init(void)
{
    uint16_t i;
    for (i = 0; i < 256; i++) {
        g_handler_table[i] = (app_comm_handler_t)0;
    }
    g_rx_state  = COMM_RX_STATE_IDLE;
    g_rx_index  = 0;
    g_rx_target = 0;
}

void app_comm_register_handler(uint8_t cmd, app_comm_handler_t handler)
{
    g_handler_table[cmd] = handler;
}

static void app_comm_dispatch(uint8_t cmd, uint8_t *data, uint16_t len)
{
    if (g_handler_table[cmd] != (app_comm_handler_t)0) {
        g_handler_table[cmd](data, len);
    }
}

void app_comm_send_frame(uint8_t cmd, uint8_t *data, uint16_t len)
{
    static uint8_t frame[APP_COMM_FRAME_LEN_MAX + 4];
    uint8_t  length;
    uint16_t crc;
    uint16_t i;
    uint16_t pos;
    uint16_t total;
    uint16_t j;

    length = (uint8_t)(len + 3U);
    frame[0] = APP_COMM_SOF;
    frame[1] = length;
    frame[2] = cmd;
    for (i = 0; i < len; i++) {
        frame[3 + i] = data[i];
    }
    crc = algo_crc16(&frame[2], (uint16_t)(len + 1U));
    pos = 3 + len;
    frame[pos]     = (uint8_t)(crc & 0xFFu);
    frame[pos + 1] = (uint8_t)(crc >> 8u);
    frame[pos + 2] = APP_COMM_EOF;
    total = (uint16_t)(length + 3U);
    for (j = 0; j < total; j++) {
        bsp_uart_send_byte(frame[j]);
    }
}

void app_comm_process_byte(uint8_t byte)
{
    uint16_t crc_calc;
    uint16_t crc_recv;
    uint8_t  cmd;
    uint16_t data_len;

    switch (g_rx_state) {
    case COMM_RX_STATE_IDLE:
        if (byte == APP_COMM_SOF) {
            g_rx_state = COMM_RX_STATE_RECV_CMD;
        }
        break;

    case COMM_RX_STATE_RECV_CMD:
        if (byte == APP_COMM_CMD_WRITE_DATA) {
            g_rx_buf[0] = byte;
            g_rx_state  = COMM_RX_STATE_RECV_WRITE;
            g_rx_target = APP_COMM_WRITE_FRAME_LEN + 1U;
            g_rx_index  = 1;
        }
        else {
            if (byte < APP_COMM_FRAME_LEN_MIN || byte > APP_COMM_FRAME_LEN_MAX) {
                g_rx_state = COMM_RX_STATE_IDLE;
            }
            else {
                g_rx_state  = COMM_RX_STATE_RECV_FRAME;
                g_rx_target = byte;
                g_rx_index  = 0;
            }
        }
        break;

    case COMM_RX_STATE_RECV_FRAME:
        g_rx_buf[g_rx_index++] = byte;
        if (g_rx_index >= g_rx_target) {
            g_rx_state = COMM_RX_STATE_WAIT_EOF;
        }
        break;

    case COMM_RX_STATE_RECV_WRITE:
        g_rx_buf[g_rx_index++] = byte;
        if (g_rx_index >= g_rx_target) {
            g_rx_state = COMM_RX_STATE_WAIT_EOF;
        }
        break;

    case COMM_RX_STATE_WAIT_EOF:
        if (byte == APP_COMM_EOF) {
            if (g_rx_target == APP_COMM_WRITE_FRAME_LEN + 1U) {
                crc_calc = algo_crc16(g_rx_buf, APP_COMM_WRITE_DATA_SIZE + 1U);
                crc_recv = (uint16_t)g_rx_buf[APP_COMM_WRITE_DATA_SIZE + 1] |
                          ((uint16_t)g_rx_buf[APP_COMM_WRITE_DATA_SIZE + 2] << 8);
                if (crc_calc == crc_recv) {
                    app_comm_dispatch(APP_COMM_CMD_WRITE_DATA,
                                      &g_rx_buf[1], APP_COMM_WRITE_DATA_SIZE);
                }
            }
            else {
                cmd      = g_rx_buf[0];
                data_len = g_rx_target - 3U;
                crc_calc = algo_crc16(g_rx_buf, data_len + 1U);
                crc_recv = (uint16_t)g_rx_buf[g_rx_target - 2] |
                          ((uint16_t)g_rx_buf[g_rx_target - 1] << 8);
                if (crc_calc == crc_recv) {
                    app_comm_dispatch(cmd, &g_rx_buf[1], data_len);
                }
            }
        }
        g_rx_state = COMM_RX_STATE_IDLE;
        g_rx_index = 0;
        break;

    default:
        g_rx_state = COMM_RX_STATE_IDLE;
        g_rx_index = 0;
        break;
    }
}

// end of file
