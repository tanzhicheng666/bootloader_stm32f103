/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_communicate.h
 *  Description: Application layer - bootloader communication protocol
 *               Frame parsing, CRC validation, command dispatch
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_COMMUNICATE_H
#define __APP_COMMUNICATE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*==============================================================================
 * Command Codes
 *============================================================================*/
#define APP_COMM_CMD_PING             0x01U
#define APP_COMM_CMD_PONG             0x02U
#define APP_COMM_CMD_ERASE            0x03U
#define APP_COMM_CMD_ERASE_ACK        0x04U
#define APP_COMM_CMD_WRITE_ADDR       0x05U
#define APP_COMM_CMD_ACK              0x06U
#define APP_COMM_CMD_VERIFY           0x07U
#define APP_COMM_CMD_VERIFY_ACK       0x08U
#define APP_COMM_CMD_RESET            0x09U
#define APP_COMM_CMD_WRITE_DATA       0x0AU

/*==============================================================================
 * Frame Constants
 *============================================================================*/
#define APP_COMM_FRAME_LEN_MIN         3U    /* min Length = CMD + CRC = 3 */
#define APP_COMM_FRAME_LEN_MAX         255U  /* max Length (uint8_t) */
#define APP_COMM_WRITE_DATA_SIZE       256U  /* fixed data size for WRITE_DATA */
#define APP_COMM_WRITE_DATA_FRAME_LEN  258U  /* WRITE_DATA: 256 data + 2 CRC */

/*==============================================================================
 * Protocol Versions
 *============================================================================*/
#define APP_COMM_VERSION_MAJOR         0x01U
#define APP_COMM_VERSION_MINOR         0x00U
#define APP_COMM_VERSION               ((APP_COMM_VERSION_MAJOR << 8) | APP_COMM_VERSION_MINOR)

/*==============================================================================
 * Status Codes
 *============================================================================*/
#define APP_COMM_STATUS_OK             0x00U
#define APP_COMM_STATUS_ERR            0x01U

/*==============================================================================
 * Command Handler Callback Type
 *============================================================================*/
typedef void (*app_comm_handler_t)(uint8_t *data, uint16_t len);

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void    app_comm_init(void);
void    app_comm_process(void);
void    app_comm_process_byte(uint8_t byte);
void    app_comm_send_frame(uint8_t cmd, uint8_t *data, uint16_t len);
void    app_comm_register_handler(uint8_t cmd, app_comm_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* __APP_COMMUNICATE_H */
