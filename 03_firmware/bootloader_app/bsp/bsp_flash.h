/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_flash.h
 *  Description: BSP Flash driver — read/write/erase with address protection
 *               Boot 16KB(pages 0~7), Param page 8, APP pages 9~127.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*==============================================================================
 * Flash Layout (STM32F103RCT6, 256KB)
 *============================================================================*/
#define BSP_FLASH_BASE              0x08000000U
#define BSP_FLASH_SIZE              0x00040000U   /* 256KB */
#define BSP_FLASH_PAGE_SIZE         0x00000800U   /* 2KB */
#define BSP_FLASH_PAGE_COUNT        128U

/*
 * Bootloader occupies pages 0~7 (16KB).
 * Parameter area is page 8 (2KB, uses 1KB).
 * APP starts from page 9.
 */
#define BSP_FLASH_BOOT_SIZE         0x00004000U   /* 16KB */
#define BSP_FLASH_PARAM_PAGE        8U
#define BSP_FLASH_PARAM_START       (BSP_FLASH_BASE + BSP_FLASH_PARAM_PAGE * BSP_FLASH_PAGE_SIZE)
#define BSP_FLASH_PARAM_END         (BSP_FLASH_PARAM_START + BSP_FLASH_PAGE_SIZE - 1U)
#define BSP_FLASH_PARAM_SIZE        BSP_FLASH_PAGE_SIZE

#define BSP_FLASH_APP_START         (BSP_FLASH_PARAM_START + BSP_FLASH_PAGE_SIZE)

/*==============================================================================
 * Return Codes
 *============================================================================*/
#define BSP_FLASH_OK                0U
#define BSP_FLASH_ERR_ADDR          1U
#define BSP_FLASH_ERR_ERASE         2U
#define BSP_FLASH_ERR_PROGRAM       3U

/*==============================================================================
 * Function Declarations
 *============================================================================*/
void    bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t bsp_flash_erase_page(uint32_t page_addr);
uint8_t bsp_flash_erase_range(uint32_t start_addr, uint32_t end_addr);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FLASH_H */

// end of file
