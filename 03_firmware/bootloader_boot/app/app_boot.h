/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_boot.h
 *  Description: Application layer — boot mode decision and APP jump
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_BOOT_H
#define __APP_BOOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Boot Mode
 *============================================================================*/
typedef enum {
    BOOT_MODE_UPGRADE   = 0,
    BOOT_MODE_JUMP_APP  = 1,
} en_boot_mode_t;

/*==============================================================================
 * Function Declarations
 *============================================================================*/
en_boot_mode_t app_boot_check(void);
void           app_boot_jump_to_app(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BOOT_H */

// end of file
