/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: main.c
 *  Description: Main entry — boot decision + upgrade mode task loop
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *  2026.6.23            v02           TimTan        Extract handlers to app_cmd
 *--------------------------------------------------
*/

#include "main.h"

/*==============================================================================
 *  Task: LED heartbeat — toggles LED_RUN every 500ms
 *============================================================================*/
static void task_led_blink(void)
{
    bsp_led_run_toggle();
}

/*==============================================================================
 *  Task: Button scan — 10ms polling with debounce
 *        KEY_LEFT / KEY_RIGHT both toggle LED_DEBUG on press
 *============================================================================*/
static void task_button_scan(void)
{
    app_button_scan();

    if (app_button_get_key_left())
    {
        bsp_led_debug_toggle();
    }

    if (app_button_get_key_right())
    {
        bsp_led_debug_toggle();
    }
}

/*==============================================================================
 *  Task: UART protocol parser — feeds all RX bytes into frame state machine
 *        Called every 1ms for low-latency command processing
 *============================================================================*/
static void task_comm_process(void)
{
    while (bsp_uart_available())
    {
        app_comm_process_byte(bsp_uart_read());
    }
}

/*==============================================================================
 *  Function: int main(void)
 *  Description: System init → boot mode decision → upgrade loop or APP jump
 *============================================================================*/
int main(void)
{
    /* -------- HAL + clock + system tick -------- */
    HAL_Init();
    bsp_sysclk_config();
    bsp_tick_init();

    /* -------- minimal peripherals for boot decision -------- */
    bsp_led_run_init();
    bsp_led_debug_init();
    bsp_key_init();
    app_button_init();

    /* -------- check: jump to APP or enter upgrade mode? -------- */
    if (app_boot_check() == BOOT_MODE_JUMP_APP)
    {
        app_boot_jump_to_app();
        /* never returns */
    }

    /* ================================================================
     *  Upgrade mode — init full communication stack and enter task loop
     * ================================================================ */
    bsp_uart_init(115200);
    app_comm_init();
    app_task_init();

    /* register all protocol command handlers */
    app_comm_register_handler(APP_COMM_CMD_PING,        comm_handler_ping);
    app_comm_register_handler(APP_COMM_CMD_ERASE,       comm_handler_erase);
    app_comm_register_handler(APP_COMM_CMD_WRITE_ADDR,  comm_handler_write_addr);
    app_comm_register_handler(APP_COMM_CMD_WRITE_DATA,  comm_handler_write_data);
    app_comm_register_handler(APP_COMM_CMD_VERIFY,      comm_handler_verify);
    app_comm_register_handler(APP_COMM_CMD_RESET,       comm_handler_reset);

    /* register periodic tasks */
    app_task_create(task_comm_process, 1);     /* 1ms  — protocol parsing */
    app_task_create(task_button_scan,  10);     /* 10ms — button scan */
    app_task_create(task_led_blink,    500);    /* 500ms — heartbeat LED */

    /* -------- main loop -------- */
    while (1)
    {
        app_task_run();
    }
}

/*==============================================================================
 *  Function: void Error_Handler(void)
 *  Description: Fatal error — infinite loop
 *============================================================================*/
void Error_Handler(void)
{
    while (1)
    {
    }
}

// end of file
