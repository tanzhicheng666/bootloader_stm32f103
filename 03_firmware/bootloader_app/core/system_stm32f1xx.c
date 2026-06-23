/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: system_stm32f1xx.c
 *  Description: SystemInit for bootloader APP — preserves clock from bootloader
 *               Only updates SystemCoreClock, does NOT reset RCC.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "stm32f1xx_hal.h"

uint32_t SystemCoreClock = 72000000U;

/* AHB/APB prescaler tables required by HAL RCC module */
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void)
{
    /* Bootloader already configured HSE(8MHz) + PLL(×9) = 72MHz.
     * Do NOT reset RCC — just update SystemCoreClock and VTOR. */
    SystemCoreClock = 72000000U;

    /* Relocate vector table to APP base */
    SCB->VTOR = 0x08004800U;

    /* Enable FPU if available (not on Cortex-M3, no-op) */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif
}

void SystemCoreClockUpdate(void)
{
    /* Just return the known value — bootloader set 72MHz */
}
