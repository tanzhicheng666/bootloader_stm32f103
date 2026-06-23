/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_flash.h
 *  Description: BSP Flash driver - read from any address,
 *               write to last two pages only
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/*==============================================================================
 * Flash Layout (STM32F103ZE, 512KB)
 *============================================================================*/
#define BSP_FLASH_BASE              0x08000000U
#define BSP_FLASH_SIZE              0x00080000U   /* 512KB */
#define BSP_FLASH_PAGE_SIZE         0x00000800U   /* 2KB */
#define BSP_FLASH_PAGE_COUNT        256U

/* Last two pages (254, 255) */
#define BSP_FLASH_PAGE_LAST         255U
#define BSP_FLASH_PAGE_SECOND_LAST  254U
#define BSP_FLASH_LAST2_START       (BSP_FLASH_BASE + BSP_FLASH_PAGE_SECOND_LAST * BSP_FLASH_PAGE_SIZE)
#define BSP_FLASH_LAST2_END         (BSP_FLASH_BASE + BSP_FLASH_SIZE - 1U)
#define BSP_FLASH_LAST2_SIZE        (2U * BSP_FLASH_PAGE_SIZE)

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void    bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t bsp_flash_erase_page(uint32_t page_addr);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FLASH_H */
