/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: main.c
 *  Description:
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/
#include "main.h"

/*==============================================================================
 * Local Defines
 *============================================================================*/
/* Last flash page (page 255) start address */
#define FLASH_PAGE_LAST_ADDR     (BSP_FLASH_BASE + BSP_FLASH_PAGE_LAST * BSP_FLASH_PAGE_SIZE)

/*==============================================================================
 * Communication Command Handlers
 *============================================================================*/

/*
 *  Handler: void comm_handler_ping(uint8_t *data, uint16_t len)
 *  Description: Respond to PING with PONG (status=OK + version).
*/
static void comm_handler_ping(uint8_t *data, uint16_t len)
{
    uint8_t resp[3];

    (void)data;
    (void)len;

    resp[0] = APP_COMM_STATUS_OK;
    resp[1] = (uint8_t)(APP_COMM_VERSION & 0xFFu);       /* version low */
    resp[2] = (uint8_t)(APP_COMM_VERSION >> 8u);          /* version high */

    app_comm_send_frame(APP_COMM_CMD_PONG, resp, 3);
    bsp_led_debug_toggle();
}

/*
 *  Handler: void comm_handler_erase(uint8_t *data, uint16_t len)
 *  Description: Respond to ERASE with ERASE_ACK (status=OK).
*/
static void comm_handler_erase(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    app_comm_send_frame(APP_COMM_CMD_ERASE_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*
 *  Handler: void comm_handler_write_addr(uint8_t *data, uint16_t len)
 *  Description: Store the write address, respond with ACK (status=OK).
*/
static void comm_handler_write_addr(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    /* TODO: store data[0..3] as flash write address */

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*
 *  Handler: void comm_handler_write_data(uint8_t *data, uint16_t len)
 *  Description: Receive 256B data block, respond with ACK (status=OK).
*/
static void comm_handler_write_data(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    /* TODO: write data[0..255] to flash at stored address */

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*
 *  Handler: void comm_handler_verify(uint8_t *data, uint16_t len)
 *  Description: Respond to VERIFY with VERIFY_ACK (status=OK).
*/
static void comm_handler_verify(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    /* TODO: read flash, compute CRC, compare */

    app_comm_send_frame(APP_COMM_CMD_VERIFY_ACK, &status, 1);
    bsp_led_debug_toggle();
}

/*
 *  Handler: void comm_handler_reset(uint8_t *data, uint16_t len)
 *  Description: Respond to RESET with ACK then perform system reset.
*/
static void comm_handler_reset(uint8_t *data, uint16_t len)
{
    uint8_t status = APP_COMM_STATUS_OK;

    (void)data;
    (void)len;

    app_comm_send_frame(APP_COMM_CMD_ACK, &status, 1);
    bsp_led_debug_toggle();

    /* Short delay to allow ACK to be sent before reset */
    HAL_Delay(10);
    NVIC_SystemReset();
}

/*==============================================================================
 * Task Functions
 *============================================================================*/

/*
 *  Function: static void task_button_scan(void)
 *  Description: Called every 10ms. Scans button state machines.
 *               KEY_LEFT:  write 0x1234 to first address of last flash page
 *               KEY_RIGHT: erase last flash page
*/
static void task_button_scan(void)
{
    uint16_t data;

    app_button_scan();

    /* KEY_LEFT: write 0x1234 to first address of last page */
    if (app_button_get_key_left())
    {
        //data = 0x1234;
        //bsp_flash_write(FLASH_PAGE_LAST_ADDR, (uint8_t *)&data, 2);
        bsp_led_debug_toggle();
    }

    /* KEY_RIGHT: erase last flash page */
    if (app_button_get_key_right())
    {
        //bsp_flash_erase_page(FLASH_PAGE_LAST_ADDR);
        bsp_led_debug_toggle();
    }
}

/*
 *  Function: static void task_heartbeat(void)
 *  Description: Called every 500ms. Toggles LED_RUN as a heartbeat indicator.
*/
static void task_heartbeat(void)
{
    bsp_led_run_toggle();
}

/*
 *  Function: static void task_comm_process(void)
 *  Description: Echo debug + raw frame counter + protocol processing.
*/
static void task_comm_process(void)
{
    static uint16_t byte_count = 0;
    uint8_t byte;

    while (bsp_uart_available())
    {
        byte = bsp_uart_read();
        bsp_uart_send_byte(byte);          /* echo for debug */
        app_comm_process_byte(byte);       /* feed to protocol parser */

        byte_count++;
        if (byte_count >= 4)
        {
            bsp_led_run_toggle();          /* LED_RUN = raw 4-byte counter */
            byte_count = 0;
        }
    }
}

/*==============================================================================
 * Main Function
 *============================================================================*/
int main(void)
{
    //uint32_t sys_clk = 0;
    // Initialize HAL library; this function must be called
    HAL_Init();

    /*
     * System clock (AHB/APB) configuration
     * When using the external high-speed clock HSE (8 MHz) as the system clock source,
     * the PLL input can be set to no division or divide-by-2. To achieve the maximum
     * system frequency of 72 MHz, no division is selected here.
     * The PLL multiplier has a maximum factor of 16. To reach 72 MHz, a multiplier of
     * 9 is chosen (8 MHz * 9 = 72 MHz).
    */
    SystemClock_Config();

    /* Initialize BSP peripherals */
    bsp_led_init();
    bsp_key_init();
    bsp_uart_init(115200);

    /* Initialize application modules */
    app_button_init();
    app_comm_init();
    app_task_init();

    /*
     * Register communication protocol command handlers
     */
    app_comm_register_handler(APP_COMM_CMD_PING,        comm_handler_ping);
    app_comm_register_handler(APP_COMM_CMD_ERASE,       comm_handler_erase);
    app_comm_register_handler(APP_COMM_CMD_WRITE_ADDR,  comm_handler_write_addr);
    app_comm_register_handler(APP_COMM_CMD_WRITE_DATA,  comm_handler_write_data);
    app_comm_register_handler(APP_COMM_CMD_VERIFY,      comm_handler_verify);
    app_comm_register_handler(APP_COMM_CMD_RESET,       comm_handler_reset);

    /*
     * Register periodic tasks:
     * - Comm process at 1ms for low-latency protocol parsing
     * - Button scan at 10ms for responsive key detection
     * - Heartbeat LED at 500ms to indicate system is running
     */
    app_task_create(task_comm_process,  1);
    app_task_create(task_button_scan,  10);
    app_task_create(task_heartbeat,   500);

    while(1)
    {
        app_task_run();
    }
}

/*
 *  Function: void Error_Handler(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Error handler function. Currently implemented as an infinite loop
 *               with no action taken.
*/
void Error_Handler(void)
{
    while(1)
    {
    }
}
