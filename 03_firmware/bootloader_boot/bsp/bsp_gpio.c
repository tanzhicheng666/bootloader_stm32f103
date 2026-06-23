/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_gpio.c
 *  Description: BSP GPIO driver — LED (RUN + DEBUG) and KEY control
 *               PB12=LED_RUN, PB13=LED_DEBUG (active-low)
 *               PB10=KEY_LEFT, PB2=KEY_RIGHT (active-low, pull-up)
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_gpio.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 * LED_RUN Interface (PB12)
 *============================================================================*/

/*==============================================================================
 *  Function: void bsp_led_run_init(void)
 *  Description: Initialize LED_RUN (PB12) as push-pull output.
 *               Active-low — forces high to keep LED off during init.
 *============================================================================*/
void bsp_led_run_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Force LED off (high = off for active-low) */
    GPIOB->BSRR = LED_RUN_PIN;

    gpio_init.Pin   = LED_RUN_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    HAL_GPIO_WritePin(LED_RUN_PORT, LED_RUN_PIN, GPIO_PIN_SET);
}

void bsp_led_run_on(void)
{
    HAL_GPIO_WritePin(LED_RUN_PORT, LED_RUN_PIN, GPIO_PIN_RESET);
}

void bsp_led_run_off(void)
{
    HAL_GPIO_WritePin(LED_RUN_PORT, LED_RUN_PIN, GPIO_PIN_SET);
}

void bsp_led_run_toggle(void)
{
    HAL_GPIO_TogglePin(LED_RUN_PORT, LED_RUN_PIN);
}

/*==============================================================================
 * LED_DEBUG Interface (PB13)
 *============================================================================*/

/*==============================================================================
 *  Function: void bsp_led_debug_init(void)
 *  Description: Initialize LED_DEBUG (PB13) as push-pull output.
 *               Active-low — forces high to keep LED off during init.
 *============================================================================*/
void bsp_led_debug_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Force LED off (high = off for active-low) */
    GPIOB->BSRR = LED_DEBUG_PIN;

    gpio_init.Pin   = LED_DEBUG_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    HAL_GPIO_WritePin(LED_DEBUG_PORT, LED_DEBUG_PIN, GPIO_PIN_SET);
}

void bsp_led_debug_on(void)
{
    HAL_GPIO_WritePin(LED_DEBUG_PORT, LED_DEBUG_PIN, GPIO_PIN_RESET);
}

void bsp_led_debug_off(void)
{
    HAL_GPIO_WritePin(LED_DEBUG_PORT, LED_DEBUG_PIN, GPIO_PIN_SET);
}

void bsp_led_debug_toggle(void)
{
    HAL_GPIO_TogglePin(LED_DEBUG_PORT, LED_DEBUG_PIN);
}

/*==============================================================================
 * KEY Interface
 *============================================================================*/

/*==============================================================================
 *  Function: void bsp_key_init(void)
 *  Description: Initialize KEY_LEFT (PB10) and KEY_RIGHT (PB2) as inputs
 *               with pull-up. Active-low: pressed = 0, released = 1.
 *============================================================================*/
void bsp_key_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin   = KEY_LEFT_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    gpio_init.Pin   = KEY_RIGHT_PIN;
    gpio_init.Mode  = GPIO_MODE_INPUT;
    gpio_init.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpio_init);
}

uint8_t bsp_key_left_read(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(KEY_LEFT_PORT, KEY_LEFT_PIN);
}

uint8_t bsp_key_right_read(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(KEY_RIGHT_PORT, KEY_RIGHT_PIN);
}

// end of file
