/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: algo_crc.h
 *  Description: Algorithm layer — CRC-16/MODBUS interface
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __ALGO_CRC_H
#define __ALGO_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Function Declarations
 *============================================================================*/
uint16_t algo_crc16(const uint8_t *data, uint16_t len);

/* Exported for incremental CRC calculation */
extern const uint16_t crc16_table[256];

#ifdef __cplusplus
}
#endif

#endif /* __ALGO_CRC_H */

// end of file
