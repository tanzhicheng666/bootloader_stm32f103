/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_button.h
 *  Description: Application layer - button press detection using state machine
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __APP_BUTTON_H
#define __APP_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*==============================================================================
 * Button State Machine States
 *============================================================================*/
typedef enum {
    APP_BUTTON_STATE_IDLE              = 0,
    APP_BUTTON_STATE_PRESS_DEBOUNCE    = 1,
    APP_BUTTON_STATE_PRESSED           = 2,
    APP_BUTTON_STATE_RELEASE_DEBOUNCE  = 3,
} app_button_state_t;

/*==============================================================================
 * Button Instance Structure
 *============================================================================*/
typedef struct {
    app_button_state_t state;
    uint8_t            debounce_cnt;
    uint8_t            event_flag;
} app_button_t;

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
