/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: app_boot.c
 *  Description: Application layer — boot mode decision and APP jump logic
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "app_boot.h"
#include "app_button.h"
#include "bsp_gpio.h"
#include "bsp_flash.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 * Local Constants
 *============================================================================*/

/* APP validity: stack pointer must be within RAM range */
#define APP_RAM_BASE        0x20000000U
#define APP_RAM_END         0x2000C000U   /* 48KB */

/* APP validity: reset vector must be within flash range */
#define APP_FLASH_BASE      0x08000000U
#define APP_FLASH_END       0x08040000U   /* 256KB */

/* Upgrade flag value written to parameter area */
#define APP_BOOT_UPGRADE_FLAG       0x5A5AA5A5U

/*==============================================================================
 *  Function: static uint8_t app_boot_is_app_valid(void)
 *  Input:  none
 *  Return: 1 = valid, 0 = invalid
 *  Description: Check if APP at BSP_FLASH_APP_START has valid stack pointer
 *               and reset vector, indicating a real application is present.
 *============================================================================*/
static uint8_t app_boot_is_app_valid(void)
{
    uint32_t sp;
    uint32_t pc;

    /* Read APP's initial stack pointer (first word of APP binary) */
    sp = *((volatile uint32_t *)BSP_FLASH_APP_START);

    /* SP must be within RAM range */
    if (sp < APP_RAM_BASE || sp >= APP_RAM_END) {
        return 0;
    }

    /* Read APP's reset vector (second word of APP binary) */
    pc = *((volatile uint32_t *)(BSP_FLASH_APP_START + 4U));

    /* Reset vector must be within flash range and thumb-bit set */
    if (pc < APP_FLASH_BASE || pc >= APP_FLASH_END) {
        return 0;
    }

    return 1;
}

/*==============================================================================
 *  Function: en_boot_mode_t app_boot_check(void)
 *  Input:  none
 *  Return: BOOT_MODE_JUMP_APP or BOOT_MODE_UPGRADE
 *  Description: Determine boot mode by checking:
 *               1. Button (KEY_LEFT) — pressed → upgrade
 *               2. Upgrade flag in parameter area — set → upgrade
 *               3. APP validity — valid → jump, invalid → upgrade
 *============================================================================*/
en_boot_mode_t app_boot_check(void)
{
    uint32_t upgrade_flag;

    if (bsp_key_left_read() == 0U) {
        return BOOT_MODE_UPGRADE;
    }

    /* Check upgrade flag in parameter area */
    bsp_flash_read(BSP_FLASH_PARAM_START, (uint8_t *)&upgrade_flag, 4);
    if (upgrade_flag == APP_BOOT_UPGRADE_FLAG) {
        return BOOT_MODE_UPGRADE;
    }

    /* Check if APP is valid */
    if (app_boot_is_app_valid()) {
        return BOOT_MODE_JUMP_APP;
    }

    /* No valid APP — stay in upgrade mode */
    return BOOT_MODE_UPGRADE;
}

/*==============================================================================
 *  Function: void app_boot_jump_to_app(void)
 *  Input:  none
 *  Return: none (does not return)
 *  Description: Jump to APP firmware at BSP_FLASH_APP_START.
 *               Disables interrupts, stops SysTick, sets MSP and jumps.
 *============================================================================*/
void app_boot_jump_to_app(void)
{
    uint32_t app_sp;
    uint32_t app_pc;
    void (*app_reset)(void);

    /* Read APP's initial SP and PC from vector table */
    app_sp = *((volatile uint32_t *)BSP_FLASH_APP_START);
    app_pc = *((volatile uint32_t *)(BSP_FLASH_APP_START + 4U));

    /* Disable global interrupts */
    __disable_irq();

    /* Stop SysTick */
    SysTick->CTRL = 0;

    /* Set the vector table offset to APP base */
    SCB->VTOR = BSP_FLASH_APP_START;

    /* Set main stack pointer to APP's initial SP */
    __set_MSP(app_sp);

    /* Jump to APP's reset handler */
    app_reset = (void (*)(void))app_pc;
    app_reset();

    /* Should never reach here */
    while (1) {
    }
}

// end of file
