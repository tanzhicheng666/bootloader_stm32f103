/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_gpio.c
 *  Description: BSP GPIO driver - LED and KEY initialization and interface functions
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_gpio.h"

/*==============================================================================
 *  Function: void bsp_led_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize LED GPIO pins (PB12: LED_RUN, PB13: LED_DEBUG)
 *               as push-pull outputs with no pull-up/down.
 *============================================================================*/
void bsp_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOB clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Force LEDs off before GPIO init to avoid glitching */
    GPIOB->BRR = LED_RUN_PIN | LED_DEBUG_PIN;

    /* Configure LED_RUN (PB12) and LED_DEBUG (PB13) as push-pull output */
    GPIO_InitStruct.Pin   = LED_RUN_PIN | LED_DEBUG_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Ensure LEDs remain off after init (active high: high = on) */
    HAL_GPIO_WritePin(GPIOB, LED_RUN_PIN | LED_DEBUG_PIN, GPIO_PIN_SET);
}

/*==============================================================================
 *  Function: void bsp_key_init(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Initialize KEY GPIO pins (PB10: KEY_LEFT, PB2: KEY_RIGHT)
 *               as inputs with pull-up. Assumes active-low (key press = GND).
 *============================================================================*/
void bsp_key_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOB clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure KEY_LEFT (PB10) */
    GPIO_InitStruct.Pin   = KEY_LEFT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure KEY_RIGHT (PB2) */
    GPIO_InitStruct.Pin   = KEY_RIGHT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/*==============================================================================
 * LED Control Interface Functions
 *============================================================================*/

/*
 *  Function: void bsp_led_run_on(void)
 *  Description: Turn LED_RUN (PB12) on.
*/
void bsp_led_run_on(void)
{
    HAL_GPIO_WritePin(LED_RUN_PORT, LED_RUN_PIN, GPIO_PIN_SET);
}

/*
 *  Function: void bsp_led_run_off(void)
 *  Description: Turn LED_RUN (PB12) off.
*/
void bsp_led_run_off(void)
{
    HAL_GPIO_WritePin(LED_RUN_PORT, LED_RUN_PIN, GPIO_PIN_RESET);
}

/*
 *  Function: void bsp_led_run_toggle(void)
 *  Description: Toggle LED_RUN (PB12) state.
*/
void bsp_led_run_toggle(void)
{
    HAL_GPIO_TogglePin(LED_RUN_PORT, LED_RUN_PIN);
}

/*
 *  Function: void bsp_led_debug_on(void)
 *  Description: Turn LED_DEBUG (PB13) on.
*/
void bsp_led_debug_on(void)
{
    HAL_GPIO_WritePin(LED_DEBUG_PORT, LED_DEBUG_PIN, GPIO_PIN_SET);
}

/*
 *  Function: void bsp_led_debug_off(void)
 *  Description: Turn LED_DEBUG (PB13) off.
*/
void bsp_led_debug_off(void)
{
    HAL_GPIO_WritePin(LED_DEBUG_PORT, LED_DEBUG_PIN, GPIO_PIN_RESET);
}

/*
 *  Function: void bsp_led_debug_toggle(void)
 *  Description: Toggle LED_DEBUG (PB13) state.
*/
void bsp_led_debug_toggle(void)
{
    HAL_GPIO_TogglePin(LED_DEBUG_PORT, LED_DEBUG_PIN);
}

/*==============================================================================
 * KEY Read Interface Functions
 *============================================================================*/

/*
 *  Function: uint8_t bsp_key_left_read(void)
 *  Input:  none
 *  Return: 0 = key pressed (active-low), 1 = key released
 *  Description: Read the state of KEY_LEFT (PB10).
*/
uint8_t bsp_key_left_read(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(KEY_LEFT_PORT, KEY_LEFT_PIN);
}

/*
 *  Function: uint8_t bsp_key_right_read(void)
 *  Input:  none
 *  Return: 0 = key pressed (active-low), 1 = key released
 *  Description: Read the state of KEY_RIGHT (PB2).
*/
uint8_t bsp_key_right_read(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(KEY_RIGHT_PORT, KEY_RIGHT_PIN);
}
