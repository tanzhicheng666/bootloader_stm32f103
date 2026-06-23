/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: stm32f1xx_hal_conf.h
 *  Description: HAL module configuration for STM32F103RCT6
 *               Only enables modules actually used by the project.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __STM32F1XX_HAL_CONF_H
#define __STM32F1XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Module Selection
 *============================================================================*/
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/*==============================================================================
 * Oscillator Values
 *============================================================================*/
#define HSE_VALUE              8000000U
#define HSE_STARTUP_TIMEOUT    100U
#define HSI_VALUE              8000000U
#define LSI_VALUE               40000U
#define LSE_VALUE               32768U
#define LSE_STARTUP_TIMEOUT    5000U

/*==============================================================================
 * System Configuration
 *============================================================================*/
#define VDD_VALUE              3300U
#define TICK_INT_PRIORITY       0x0FU
#define USE_RTOS                0U
#define PREFETCH_ENABLE         1U

/*==============================================================================
 * Register Callbacks (disabled for minimal footprint)
 *============================================================================*/
#define USE_HAL_ADC_REGISTER_CALLBACKS          0U
#define USE_HAL_CAN_REGISTER_CALLBACKS          0U
#define USE_HAL_CEC_REGISTER_CALLBACKS          0U
#define USE_HAL_DAC_REGISTER_CALLBACKS          0U
#define USE_HAL_ETH_REGISTER_CALLBACKS          0U
#define USE_HAL_HCD_REGISTER_CALLBACKS          0U
#define USE_HAL_I2C_REGISTER_CALLBACKS          0U
#define USE_HAL_I2S_REGISTER_CALLBACKS          0U
#define USE_HAL_MMC_REGISTER_CALLBACKS          0U
#define USE_HAL_NAND_REGISTER_CALLBACKS         0U
#define USE_HAL_NOR_REGISTER_CALLBACKS          0U
#define USE_HAL_PCCARD_REGISTER_CALLBACKS       0U
#define USE_HAL_PCD_REGISTER_CALLBACKS          0U
#define USE_HAL_RTC_REGISTER_CALLBACKS          0U
#define USE_HAL_SD_REGISTER_CALLBACKS           0U
#define USE_HAL_SMARTCARD_REGISTER_CALLBACKS    0U
#define USE_HAL_IRDA_REGISTER_CALLBACKS         0U
#define USE_HAL_SRAM_REGISTER_CALLBACKS         0U
#define USE_HAL_SPI_REGISTER_CALLBACKS          0U
#define USE_HAL_TIM_REGISTER_CALLBACKS          0U
#define USE_HAL_UART_REGISTER_CALLBACKS         0U
#define USE_HAL_USART_REGISTER_CALLBACKS        0U
#define USE_HAL_WWDG_REGISTER_CALLBACKS         0U

/*==============================================================================
 * Module Headers
 *============================================================================*/
#ifdef HAL_RCC_MODULE_ENABLED
 #include "stm32f1xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
 #include "stm32f1xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
 #include "stm32f1xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
 #include "stm32f1xx_hal_cortex.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
 #include "stm32f1xx_hal_adc.h"
#endif

#ifdef HAL_CAN_MODULE_ENABLED
 #include "stm32f1xx_hal_can.h"
#endif

#ifdef HAL_CEC_MODULE_ENABLED
 #include "stm32f1xx_hal_cec.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
 #include "stm32f1xx_hal_crc.h"
#endif

#ifdef HAL_DAC_MODULE_ENABLED
 #include "stm32f1xx_hal_dac.h"
#endif

#ifdef HAL_ETH_MODULE_ENABLED
 #include "stm32f1xx_hal_eth.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
 #include "stm32f1xx_hal_flash.h"
#endif

#ifdef HAL_SRAM_MODULE_ENABLED
 #include "stm32f1xx_hal_sram.h"
#endif

#ifdef HAL_NOR_MODULE_ENABLED
 #include "stm32f1xx_hal_nor.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
 #include "stm32f1xx_hal_i2c.h"
#endif

#ifdef HAL_I2S_MODULE_ENABLED
 #include "stm32f1xx_hal_i2s.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
 #include "stm32f1xx_hal_iwdg.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
 #include "stm32f1xx_hal_pwr.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
 #include "stm32f1xx_hal_rtc.h"
#endif

#ifdef HAL_PCCARD_MODULE_ENABLED
 #include "stm32f1xx_hal_pccard.h"
#endif

#ifdef HAL_SD_MODULE_ENABLED
 #include "stm32f1xx_hal_sd.h"
#endif

#ifdef HAL_NAND_MODULE_ENABLED
 #include "stm32f1xx_hal_nand.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
 #include "stm32f1xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
 #include "stm32f1xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
 #include "stm32f1xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
 #include "stm32f1xx_hal_usart.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
 #include "stm32f1xx_hal_irda.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
 #include "stm32f1xx_hal_smartcard.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
 #include "stm32f1xx_hal_wwdg.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
 #include "stm32f1xx_hal_pcd.h"
#endif

#ifdef HAL_HCD_MODULE_ENABLED
 #include "stm32f1xx_hal_hcd.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
 #include "stm32f1xx_hal_exti.h"
#endif

/*==============================================================================
 * Assert
 *============================================================================*/
#ifdef USE_FULL_ASSERT
 #define assert_param(expr)  ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
 void assert_failed(uint8_t *file, uint32_t line);
#else
 #define assert_param(expr)  ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1XX_HAL_CONF_H */

// end of file
