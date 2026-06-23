/*  Copyright (c) 2026 TimTan
 *  All rights reserved
 *
 *  File name: bsp_flash.c
 *  Description: BSP Flash driver - memory-mapped read from any address,
 *               write with auto-erase to last two pages only.
 *
 *  Revision history     Version       Author        Description
 *--------------------------------------------------
 *  2026.6.20            v01           TimTan        Create file
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
 *               Flash is memory-mapped, so this is a simple byte copy.
 *============================================================================*/
void bsp_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if (buf == (uint8_t *)0 || len == 0)
    {
        return;
    }

    for (i = 0; i < len; i++)
    {
        buf[i] = *((volatile uint8_t *)(addr + i));
    }
}

/*==============================================================================
 *  Function: static uint8_t bsp_flash_erase_pages(uint32_t start_addr,
 *                                                  uint32_t end_addr)
 *  Input:  start_addr - start of erase range (must be page-aligned)
 *          end_addr   - end of erase range
 *  Return: 0 = success, 1 = error
 *  Description: Erase flash pages covering the given address range.
 *============================================================================*/
static uint8_t bsp_flash_erase_pages(uint32_t start_addr, uint32_t end_addr)
{
    FLASH_EraseInitTypeDef erase_init;
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

    if (status != HAL_OK)
    {
        return 1;
    }
    return 0;
}

/*==============================================================================
 *  Function: uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
 *  Input:  addr - destination address in flash (must be within last 2 pages)
 *          buf  - source data buffer
 *          len  - number of bytes to write
 *  Return: 0 = success
 *          1 = address out of range (not in last 2 pages)
 *          2 = erase error
 *          3 = program error
 *  Description: Write data to flash within the last 2 pages.
 *               Automatically erases affected pages before writing.
 *               Programs half-word (16-bit) at a time.
 *============================================================================*/
uint8_t bsp_flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    uint32_t page_start;
    uint32_t write_end;
    uint16_t data;
    HAL_StatusTypeDef status;

    if (buf == (uint8_t *)0 || len == 0)
    {
        return 0;
    }

    /* Validate address range: must be within last 2 pages */
    write_end = addr + len - 1U;
    if ((addr < BSP_FLASH_LAST2_START) || (write_end > BSP_FLASH_LAST2_END))
    {
        return 1;
    }

    /* Erase affected pages */
    page_start = addr & ~(BSP_FLASH_PAGE_SIZE - 1U);
    if (bsp_flash_erase_pages(page_start, write_end) != 0)
    {
        return 2;
    }

    /* Program half-word by half-word */
    HAL_FLASH_Unlock();

    for (i = 0; i < len; i += 2)
    {
        /* Pack two bytes into a half-word (little-endian) */
        data = buf[i];
        if ((i + 1) < len)
        {
            data |= ((uint16_t)buf[i + 1] << 8);
        }
        else
        {
            /* Last odd byte: pad with 0xFF (flash erased state) */
            data |= 0xFF00U;
        }

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, data);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 3;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

/*==============================================================================
 *  Function: uint8_t bsp_flash_erase_page(uint32_t page_addr)
 *  Input:  page_addr - start address of the page to erase (page-aligned)
 *  Return: 0 = success, 1 = address out of range, 2 = erase error
 *  Description: Erase a single flash page.
 *               Only pages within the last 2 pages are allowed.
 *============================================================================*/
uint8_t bsp_flash_erase_page(uint32_t page_addr)
{
    uint32_t page_end;

    /* Page-align the address */
    page_addr &= ~(BSP_FLASH_PAGE_SIZE - 1U);

    /* Validate: must be within last 2 pages */
    if ((page_addr < BSP_FLASH_LAST2_START) || (page_addr > BSP_FLASH_LAST2_END))
    {
        return 1;
    }

    page_end = page_addr + BSP_FLASH_PAGE_SIZE - 1U;
    return bsp_flash_erase_pages(page_addr, page_end);
}
