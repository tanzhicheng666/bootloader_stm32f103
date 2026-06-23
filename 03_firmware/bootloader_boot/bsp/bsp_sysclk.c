/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_sysclk.c
 *  Description: BSP system clock configuration
 *               HSE 8MHz → PLL ×9 → SYSCLK 72MHz
 *               AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_sysclk.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 *  Function: void bsp_sysclk_config(void)
 *  Input:  none
 *  Output: none
 *  Return: none
 *  Description: Configure system clock to 72MHz using HSE (8MHz) + PLL.
 *               If HSE fails, hangs in infinite loop.
 *============================================================================*/
void bsp_sysclk_config(void)
{
    RCC_OscInitTypeDef osc_init = {0};
    RCC_ClkInitTypeDef clk_init = {0};

    /* HSE oscillator: 8MHz, no division */
    osc_init.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState            = RCC_HSE_ON;
    osc_init.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    osc_init.HSIState            = RCC_HSI_ON;
    osc_init.PLL.PLLState        = RCC_PLL_ON;
    osc_init.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLMUL          = RCC_PLL_MUL9;    /* 8MHz × 9 = 72MHz */

    if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {
        /* HSE startup failure - trap here */
        while (1) {
        }
    }

    /* SYSCLK = PLLCLK = 72MHz, AHB = 72MHz, APB1 = 36MHz, APB2 = 72MHz */
    clk_init.ClockType            = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource         = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider        = RCC_SYSCLK_DIV1;
    clk_init.APB1CLKDivider       = RCC_HCLK_DIV2;
    clk_init.APB2CLKDivider       = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_2) != HAL_OK) {
        while (1) {
        }
    }
}

// end of file
