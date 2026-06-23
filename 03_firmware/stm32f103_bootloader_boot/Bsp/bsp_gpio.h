/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_gpio.h
 *  Description: BSP GPIO header file - LED and KEY pin definitions and interfaces
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/*==============================================================================
 * LED Pin Definitions
 *============================================================================*/
#define LED_RUN_PORT                GPIOB
#define LED_RUN_PIN                 GPIO_PIN_12

#define LED_DEBUG_PORT              GPIOB
#define LED_DEBUG_PIN               GPIO_PIN_13

/*==============================================================================
 * KEY Pin Definitions
 *============================================================================*/
#define KEY_LEFT_PORT               GPIOB
#define KEY_LEFT_PIN                GPIO_PIN_10

#define KEY_RIGHT_PORT              GPIOB
#define KEY_RIGHT_PIN               GPIO_PIN_2

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void bsp_led_init(void);
void bsp_key_init(void);

void bsp_led_run_on(void);
void bsp_led_run_off(void);
void bsp_led_run_toggle(void);

void bsp_led_debug_on(void);
void bsp_led_debug_off(void);
void bsp_led_debug_toggle(void);

uint8_t bsp_key_left_read(void);
uint8_t bsp_key_right_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_H */
