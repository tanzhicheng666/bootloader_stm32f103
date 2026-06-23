/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_uart.h
 *  Description: BSP UART driver - interrupt-based TX/RX with ring buffer
 *               and application-layer callback interface
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_UART_H
#define __BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*==============================================================================
 * Constants
 *============================================================================*/
#define BSP_UART_RX_BUF_SIZE    256U

/*==============================================================================
 * Callback Type
 *============================================================================*/
typedef void (*bsp_uart_rx_cb_t)(uint8_t byte);

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void     bsp_uart_init(uint32_t baudrate);
void     bsp_uart_send(uint8_t *p_data, uint16_t len);
void     bsp_uart_send_byte(uint8_t byte);
void     bsp_uart_register_rx_cb(bsp_uart_rx_cb_t callback);
uint16_t bsp_uart_available(void);
uint8_t  bsp_uart_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H */
