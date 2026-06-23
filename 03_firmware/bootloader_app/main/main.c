/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: main.c
 *  Description: Bootloader APP — LED blink at 200ms
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "main.h"

/*==============================================================================
 *  Function: int main(void)
 *============================================================================*/
int main(void)
{
    uint32_t flag;
    uint32_t led_last;
    uint32_t key_down_tick;
    uint8_t  key_was_down;

    SCB->VTOR = 0x08004800U;
    HAL_Init();
    __enable_irq();
    bsp_tick_init();
    bsp_led_run_init();
    bsp_key_init();

    led_last = bsp_tick_get();
    key_was_down = 0;

    while (1) {
        uint32_t now = bsp_tick_get();

        /* LED blink every 200ms */
        if ((now - led_last) >= 200) {
            led_last = now;
            bsp_led_run_toggle();
        }

        /* KEY_LEFT hold 1s → re-enter bootloader */
        if (bsp_key_left_read() == 0U) {
            if (key_was_down == 0) {
                key_was_down = 1;
                key_down_tick = now;
            }
            else if ((now - key_down_tick) >= 1000) {
                flag = 0x5A5AA5A5U;
                bsp_flash_erase_page(BSP_FLASH_PARAM_START);
                bsp_flash_write(BSP_FLASH_PARAM_START, (uint8_t *)&flag, 4);
                NVIC_SystemReset();
            }
        }
        else {
            key_was_down = 0;
        }
    }
}

void Error_Handler(void)
{
    while (1) {
    }
}

// end of file
