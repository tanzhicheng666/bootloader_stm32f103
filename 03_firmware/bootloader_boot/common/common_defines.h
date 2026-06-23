/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: common_defines.h
 *  Description: Common macro definitions for all projects
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __COMMON_DEFINES_H
#define __COMMON_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Bit Manipulation
 *============================================================================*/
#define BV(n)                ((uint32_t)1u << (n))
#define BV0                  (((uint32_t)1u) << 0)

/*==============================================================================
 * Min / Max / Array Size
 *============================================================================*/
#define MINIMUM(x, y)        ((x) < (y) ? (x) : (y))
#define MAXIMUM(x, y)        ((x) > (y) ? (x) : (y))
#define ARRAY_SIZE(arr)      (sizeof(arr) / sizeof((arr)[0]))

/*==============================================================================
 * Register Bit Manipulation
 *============================================================================*/
#define REGBITS_SET(reg, mask)    ((reg) |= (mask))
#define REGBITS_CLR(reg, mask)    ((reg) &= ~(mask))

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_DEFINES_H */

// end of file
