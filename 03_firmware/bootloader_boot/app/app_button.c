/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_button.c
 *  Description: Application layer — button press detection state machine
 *               Four-state debounce: IDLE → PRESS_DEBOUNCE → PRESSED →
 *               RELEASE_DEBOUNCE → IDLE (event set on complete cycle)
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_button.h"
#include "bsp_gpio.h"

/*==============================================================================
 * Local Constants
 *============================================================================*/

/* Debounce threshold: scan() is called every 10ms, so 3 counts ≈ 30ms */
#define APP_BUTTON_DEBOUNCE_THRESHOLD   3U

/*==============================================================================
 * Button State Machine States
 *============================================================================*/
typedef enum {
    BUTTON_STATE_IDLE              = 0,
    BUTTON_STATE_PRESS_DEBOUNCE    = 1,
    BUTTON_STATE_PRESSED           = 2,
    BUTTON_STATE_RELEASE_DEBOUNCE  = 3,
} en_button_state_t;

/*==============================================================================
 * Button Instance Structure
 *============================================================================*/
typedef struct {
    en_button_state_t state;
    uint8_t           debounce_cnt;
    uint8_t           event_flag;
} stc_button_t;

/*==============================================================================
 * Local Variables
 *============================================================================*/
static stc_button_t g_button_left;
static stc_button_t g_button_right;

/*==============================================================================
 *  Function: void app_button_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize button state machines.
 *               Call once during system initialization.
 *============================================================================*/
void app_button_init(void)
{
    g_button_left.state        = BUTTON_STATE_IDLE;
    g_button_left.debounce_cnt = 0;
    g_button_left.event_flag   = 0;

    g_button_right.state        = BUTTON_STATE_IDLE;
    g_button_right.debounce_cnt = 0;
    g_button_right.event_flag   = 0;
}

/*==============================================================================
 *  Function: static void app_button_fsm(stc_button_t *p_btn, uint8_t key_level)
 *  Input:  p_btn     - pointer to button instance
 *          key_level - GPIO read level (0 = pressed, 1 = released)
 *  Output: none
 *  Return: none
 *  Description: Run the button state machine for one cycle.
 *
 *  State transition:
 *    IDLE ── key=0 ──→ PRESS_DEBOUNCE ── key=0, cnt>=thr ──→ PRESSED
 *      ↑                  │                                      │
 *      │  key=1           │  key=1 (glitch) ──→ IDLE             │ key=1
 *      │                  │                                      │
 *      └── event=1 ── RELEASE_DEBOUNCE ←── key=1 ───────────────┘
 *                           │
 *                           │ key=0 (glitch) ──→ PRESSED
 *                           │ key=1, cnt>=thr → IDLE (event set)
 *============================================================================*/
static void app_button_fsm(stc_button_t *p_btn, uint8_t key_level)
{
    switch (p_btn->state) {
    case BUTTON_STATE_IDLE:
        if (key_level == 0U) {
            p_btn->state        = BUTTON_STATE_PRESS_DEBOUNCE;
            p_btn->debounce_cnt = 0;
        }
        break;

    case BUTTON_STATE_PRESS_DEBOUNCE:
        if (key_level == 0U) {
            p_btn->debounce_cnt++;
            if (p_btn->debounce_cnt >= APP_BUTTON_DEBOUNCE_THRESHOLD) {
                p_btn->state = BUTTON_STATE_PRESSED;
            }
        }
        else {
            /* Glitch — back to idle */
            p_btn->state = BUTTON_STATE_IDLE;
        }
        break;

    case BUTTON_STATE_PRESSED:
        if (key_level == 1U) {
            p_btn->state        = BUTTON_STATE_RELEASE_DEBOUNCE;
            p_btn->debounce_cnt = 0;
        }
        break;

    case BUTTON_STATE_RELEASE_DEBOUNCE:
        if (key_level == 1U) {
            p_btn->debounce_cnt++;
            if (p_btn->debounce_cnt >= APP_BUTTON_DEBOUNCE_THRESHOLD) {
                p_btn->state      = BUTTON_STATE_IDLE;
                p_btn->event_flag = 1;
            }
        }
        else {
            /* Glitch — back to pressed */
            p_btn->state = BUTTON_STATE_PRESSED;
        }
        break;

    default:
        p_btn->state = BUTTON_STATE_IDLE;
        break;
    }
}

/*==============================================================================
 *  Function: void app_button_scan(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Scan all buttons and run their state machines.
 *               Call periodically (every 10ms) from a task.
 *============================================================================*/
void app_button_scan(void)
{
    uint8_t key_left_level;
    uint8_t key_right_level;

    key_left_level  = bsp_key_left_read();
    key_right_level = bsp_key_right_read();

    app_button_fsm(&g_button_left,  key_left_level);
    app_button_fsm(&g_button_right, key_right_level);
}

/*==============================================================================
 *  Function: uint8_t app_button_get_key_left(void)
 *  Input:  none
 *  Return: 1 = valid press+release event detected, 0 = no event
 *  Description: Check and clear the KEY_LEFT event flag.
 *               Returns 1 once per complete press+release cycle.
 *============================================================================*/
uint8_t app_button_get_key_left(void)
{
    uint8_t ret;

    ret = g_button_left.event_flag;
    g_button_left.event_flag = 0;
    return ret;
}

/*==============================================================================
 *  Function: uint8_t app_button_get_key_right(void)
 *  Input:  none
 *  Return: 1 = valid press+release event detected, 0 = no event
 *  Description: Check and clear the KEY_RIGHT event flag.
 *               Returns 1 once per complete press+release cycle.
 *============================================================================*/
uint8_t app_button_get_key_right(void)
{
    uint8_t ret;

    ret = g_button_right.event_flag;
    g_button_right.event_flag = 0;
    return ret;
}

// end of file
