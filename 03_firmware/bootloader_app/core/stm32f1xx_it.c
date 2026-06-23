/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: stm32f1xx_it.c
 *  Description: Interrupt service routines for APP
 *               SysTick_Handler -> HAL_IncTick() + bsp_tick_inc()
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "bsp_tick.h"

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
    bsp_tick_inc();
}

// end of file
