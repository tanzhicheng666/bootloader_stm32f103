/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_flash.c
 *  Description: BSP Flash driver — memory-mapped read, half-word program,
 *               page erase. Only last 2 pages (126, 127) are writable
 *               to protect bootloader code.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.22            v01           TimTan        Create file
 *--------------------------------------------------
*/

#include "bsp_flash.h"
#include "stm32f1xx_hal.h"

/*==============================================================================
 *  Function: void bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
 *  Input:  addr - source address in flash
 *          buf  - destination buffer
 *          len  - number of bytes to read
 *  Output: buf filled with flash data
 *  Return: none
 *  Description: Read bytes from any flash address.
 *               Flash is memory-mapped; simple byte copy suffices.
 *============================================================================*/
void bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if (buf == (uint8_t *)0 || len == 0) {
        return;
    }

    for (i = 0; i < len; i++) {
        buf[i] = *((volatile uint8_t *)(addr + i));
    }
}

/*==============================================================================
 *  Function: static uint8_t bsp_flash_erase_pages(uint32_t start_addr,
 *                                                  uint32_t end_addr)
 *  Input:  start_addr - page-aligned start address
 *          end_addr   - last address of erase range (inclusive)
 *  Return: BSP_FLASH_OK or BSP_FLASH_ERR_ERASE
 *  Description: Erase flash pages covering the given address range.
 *============================================================================*/
static uint8_t bsp_flash_erase_pages(uint32_t start_addr, uint32_t end_addr)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    uint32_t page_error;
    uint32_t first_page;
    uint32_t num_pages;
    HAL_StatusTypeDef status;

    first_page = (start_addr - BSP_FLASH_BASE) / BSP_FLASH_PAGE_SIZE;
    num_pages  = ((end_addr - start_addr) / BSP_FLASH_PAGE_SIZE) + 1U;

    erase_init.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = start_addr;
    erase_init.NbPages     = num_pages;

    HAL_FLASH_Unlock();
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    if (status != HAL_OK) {
        return BSP_FLASH_ERR_ERASE;
    }
    return BSP_FLASH_OK;
}

/*==============================================================================
 *  Function: uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
 *  Input:  addr - destination address (must be within last 2 pages)
 *          buf  - source data buffer
 *          len  - number of bytes to write
 *  Return: BSP_FLASH_OK, BSP_FLASH_ERR_ADDR, BSP_FLASH_ERR_ERASE,
 *          or BSP_FLASH_ERR_PROGRAM
 *  Description: Write data to flash within the last 2 pages (126, 127).
 *               Automatically erases affected pages before writing.
 *               Programs half-word (16-bit) at a time.
 *============================================================================*/
uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint32_t write_end;
    uint16_t data;
    HAL_StatusTypeDef status;

    if (buf == (uint8_t *)0 || len == 0) {
        return BSP_FLASH_OK;
    }

    /* Validate address range */
    write_end = addr + len - 1U;
    if ((addr < BSP_FLASH_PARAM_START) || (write_end > (BSP_FLASH_BASE + BSP_FLASH_SIZE - 1U))) {
        return BSP_FLASH_ERR_ADDR;
    }

    /* Program half-word by half-word (caller must erase first via ERASE command) */
    HAL_FLASH_Unlock();

    for (i = 0; i < len; i += 2) {
        data = buf[i];
        if ((i + 1) < len) {
            data |= ((uint16_t)buf[i + 1] << 8);
        }
        else {
            /* Last odd byte: pad with 0xFF (flash erased state) */
            data |= 0xFF00U;
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, data);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return BSP_FLASH_ERR_PROGRAM;
        }
    }

    HAL_FLASH_Lock();
    return BSP_FLASH_OK;
}

/*==============================================================================
 *  Function: uint8_t bsp_flash_erase_page(uint32_t page_addr)
 *  Input:  page_addr - start address of the page to erase (auto-aligned)
 *  Return: BSP_FLASH_OK, BSP_FLASH_ERR_ADDR, or BSP_FLASH_ERR_ERASE
 *  Description: Erase a single flash page.
 *               Only pages 126 and 127 are permitted.
 *============================================================================*/
uint8_t bsp_flash_erase_page(uint32_t page_addr)
{
    uint32_t page_end;

    /* Page-align the address */
    page_addr &= ~(BSP_FLASH_PAGE_SIZE - 1U);

    /* Validate: must be within last 2 pages */
    if ((page_addr < BSP_FLASH_PARAM_START) || (page_addr > (BSP_FLASH_BASE + BSP_FLASH_SIZE - 1U))) {
        return BSP_FLASH_ERR_ADDR;
    }

    page_end = page_addr + BSP_FLASH_PAGE_SIZE - 1U;
    return bsp_flash_erase_pages(page_addr, page_end);
}

/*==============================================================================
 *  Function: uint8_t bsp_flash_erase_range(uint32_t start_addr, uint32_t end_addr)
 *  Input:  start_addr - start address (auto-aligned to page boundary)
 *          end_addr   - end address (inclusive)
 *  Return: BSP_FLASH_OK or error code
 *  Description: Erase all flash pages covering the specified address range.
 *               Only addresses >= BSP_FLASH_PARAM_START are permitted.
 *============================================================================*/
uint8_t bsp_flash_erase_range(uint32_t start_addr, uint32_t end_addr)
{
    /* Page-align the start address */
    start_addr &= ~(BSP_FLASH_PAGE_SIZE - 1U);

    /* Validate */
    if ((start_addr < BSP_FLASH_PARAM_START) ||
        (end_addr > (BSP_FLASH_BASE + BSP_FLASH_SIZE - 1U)))
    {
        return BSP_FLASH_ERR_ADDR;
    }

    return bsp_flash_erase_pages(start_addr, end_addr);
}

// end of file
