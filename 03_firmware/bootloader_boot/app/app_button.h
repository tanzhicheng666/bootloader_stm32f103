/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_button.h
 *  Description: Application layer — button press detection interface
 *               State machine with debounce, detects press+release events
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_BUTTON_H
#define __APP_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void    app_button_init(void);
void    app_button_scan(void);
uint8_t app_button_get_key_left(void);
uint8_t app_button_get_key_right(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BUTTON_H */

// end of file
