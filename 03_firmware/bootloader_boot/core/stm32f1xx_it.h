/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: stm32f1xx_it.h
 *  Description: Interrupt handler declarations
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __STM32F1XX_IT_H
#define __STM32F1XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Cortex-M3 Exception Handlers
 *============================================================================*/
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1XX_IT_H */

// end of file
