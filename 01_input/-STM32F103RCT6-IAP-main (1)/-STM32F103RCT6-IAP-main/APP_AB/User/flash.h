#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

//falsh写入 注意len是以word为单位
void flash_write(uint32_t addr,uint32_t*data,uint32_t len);
//flash擦除 每次擦除2K
void flash_erase(uint32_t page_addr);


#endif
