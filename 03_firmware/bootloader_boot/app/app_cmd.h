/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_cmd.h
 *  Description: Bootloader command handlers — PING/ERASE/WRITE/VERIFY/RESET
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.23            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_CMD_H
#define __APP_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Shared Write Address (used by WRITE_ADDR and WRITE_DATA handlers)
 *============================================================================*/
extern uint32_t g_write_addr;

/*==============================================================================
 * Command Handler Declarations
 *============================================================================*/
void comm_handler_ping(uint8_t *data, uint16_t len);
void comm_handler_erase(uint8_t *data, uint16_t len);
void comm_handler_write_addr(uint8_t *data, uint16_t len);
void comm_handler_write_data(uint8_t *data, uint16_t len);
void comm_handler_verify(uint8_t *data, uint16_t len);
void comm_handler_reset(uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __APP_CMD_H */

// end of file
