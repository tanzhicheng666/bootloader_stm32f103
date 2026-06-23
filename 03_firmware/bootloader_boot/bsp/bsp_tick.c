/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_tick.c
 *  Description: BSP system tick — 1ms timer using SysTick
 *               Provides bsp_tick_get() as HAL_GetTick() alternative
 *               so App layer never calls HAL directly.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_tick.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 * Local Variables
 *============================================================================*/
static volatile uint32_t g_tick_ms;

/*==============================================================================
 *  Function: void bsp_tick_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize SysTick to generate 1ms interrupts.
 *               Priority is set to the lowest urgency (highest value).
 *============================================================================*/
void bsp_tick_init(void)
{
    g_tick_ms = 0;

    /* SysTick configured for 1ms period at 72MHz HCLK */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000U);

    /* Set SysTick priority to lowest urgency */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/*==============================================================================
 *  Function: uint32_t bsp_tick_get(void)
 *  Input:  none
 *  Return: system tick count in milliseconds
 *  Description: Return ms elapsed since bsp_tick_init().
 *               This is the App-layer alternative to HAL_GetTick().
 *============================================================================*/
uint32_t bsp_tick_get(void)
{
    return g_tick_ms;
}

/*==============================================================================
 *  Function: void bsp_tick_delay_ms(uint32_t ms)
 *  Input:  ms - delay time in milliseconds
 *  Output: none
 *  Return: none
 *  Description: Blocking delay using system tick.
 *               Uses polling, suitable for short delays (< 1000ms).
 *============================================================================*/
void bsp_tick_delay_ms(uint32_t ms)
{
    uint32_t start;

    start = g_tick_ms;
    while ((g_tick_ms - start) < ms) {
        /* wait */
    }
}

/*==============================================================================
 *  Function: void bsp_tick_inc(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Increment tick counter. Called from SysTick_Handler in
 *               core/stm32f1xx_it.c. Separates ISR wiring (core layer)
 *               from tick logic (BSP layer).
 *============================================================================*/
void bsp_tick_inc(void)
{
    g_tick_ms++;
}

// end of file
