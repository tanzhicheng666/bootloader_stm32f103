/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_uart.h
 *  Description: BSP UART driver interface — USART2 interrupt-based TX/RX
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_UART_H
#define __BSP_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void     bsp_uart_init(uint32_t baudrate);
void     bsp_uart_send(uint8_t *p_data, uint16_t len);
void     bsp_uart_send_byte(uint8_t byte);
uint16_t bsp_uart_available(void);
uint8_t  bsp_uart_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_UART_H */

// end of file
