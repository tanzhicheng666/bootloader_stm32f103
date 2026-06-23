/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: common_types.h
 *  Description: Common data type redefinitions for all projects
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*==============================================================================
 * Boolean Type
 *============================================================================*/
typedef uint8_t boolean_t;

#define FALSE    ((boolean_t)0u)
#define TRUE     ((boolean_t)1u)

/*==============================================================================
 * Common Function Pointer Types
 *============================================================================*/
typedef void (*func_ptr_t)(void);
typedef void (*func_ptr_arg1_t)(uint8_t param);

/*==============================================================================
 * Status Type
 *============================================================================*/
typedef enum {
    STATUS_OK    = 0,
    STATUS_ERROR = 1,
    STATUS_BUSY  = 2,
    STATUS_TIMEOUT = 3,
} en_status_t;

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_TYPES_H */

// end of file
