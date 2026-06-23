/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_tick.h
 *  Description: BSP system tick interface — 1ms timer based on SysTick
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_TICK_H
#define __BSP_TICK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void     bsp_tick_init(void);
uint32_t bsp_tick_get(void);
void     bsp_tick_delay_ms(uint32_t ms);
void     bsp_tick_inc(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TICK_H */

// end of file
