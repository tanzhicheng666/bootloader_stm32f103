/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: stm32f1xx_it.c
 *  Description: Interrupt service routines
 *               SysTick_Handler → HAL_IncTick() + bsp_tick_inc()
 *               USART2_IRQHandler → UART TX/RX interrupt processing
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "bsp_tick.h"

/* External: UART handle from bsp_uart.c */
extern UART_HandleTypeDef g_uart_handle;

/*==============================================================================
 * Cortex-M3 Exception Handlers
 *============================================================================*/

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1) {
    }
}

void MemManage_Handler(void)
{
    while (1) {
    }
}

void BusFault_Handler(void)
{
    while (1) {
    }
}

void UsageFault_Handler(void)
{
    while (1) {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/*==============================================================================
 *  Function: void SysTick_Handler(void)
 *  Description: SysTick ISR — increments HAL tick and BSP tick counter.
 *               bsp_tick_inc() feeds the BSP layer tick that App layer uses
 *               via bsp_tick_get(), avoiding direct HAL calls from App.
 *============================================================================*/
void SysTick_Handler(void)
{
    HAL_IncTick();
    bsp_tick_inc();
}

/*==============================================================================
 *  Function: void USART2_IRQHandler(void)
 *  Description: USART2 interrupt handler — delegates to HAL UART IRQ
 *               processing which calls RxCplt/TxCplt/Error callbacks.
 *============================================================================*/
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart_handle);
}

// end of file
