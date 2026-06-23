#include "flash.h"
#include "stm32f1xx_hal.h"

void flash_write(uint32_t addr,uint32_t*data,uint32_t len)
{
	HAL_FLASH_Unlock();
	
	for(uint32_t i=0;i<len;i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,addr+i*4,data[i]);
	}
	
	HAL_FLASH_Lock();
		
}

//擦除页
void flash_erase(uint32_t page_addr)
{
	FLASH_EraseInitTypeDef erase;
	uint32_t page_error;
	
	erase.TypeErase=FLASH_TYPEERASE_PAGES;
	erase.PageAddress=page_addr;
	erase.NbPages=1;
	
	HAL_FLASH_Unlock();
	HAL_FLASHEx_Erase(&erase,&page_error);
	HAL_FLASH_Lock();
	
	
}

