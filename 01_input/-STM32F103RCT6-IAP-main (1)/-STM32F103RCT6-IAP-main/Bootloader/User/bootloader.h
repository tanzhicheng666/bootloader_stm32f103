#ifndef BOOTLODER_H
#define BOOTLODER_H

//#include "stm32f1xx_hal.h"
#include "stdint.h"
#include <stdbool.h>

//最大回滚次数限制
#define BOOT_ROLLBACK_COUNT 3

//定义bootinfo结构体

typedef enum app_id_t{
	APP_NONE,
	APP_A,
	APP_B,
	}app_id_t;//APP分区

typedef uint16_t ota_state_t;
	
#define    OTA_STATE_NORMAL  	 0x0A0A
#define    OTA_STATE_PENDING 	 0x0B0B
#define    OTA_STATE_TESTING 	 0x0C0C
#define    OTA_STATE_CONFIRM 	 0x0D0D
	
//typedef enum {
//    OTA_STATE_NORMAL  = 0x0A0A,
//    OTA_STATE_PENDING = 0x0B0B,
//    OTA_STATE_TESTING = 0x0C0C,
//    OTA_STATE_CONFIRM = 0x0D0D,
//} ota_state_t;


typedef struct boot_info_t{

	uint32_t magic;	//标志是否合法
	uint32_t active_app;//当前运行那个APP
	uint32_t pending_app;//等待切换的APP
	uint32_t pre_app;//上一次运行的APP
	ota_state_t state;//标记当前固件状态
	uint32_t boot_count;//连续启动次数
	uint32_t app_a_crc;//A校验
	uint32_t app_b_crc;//B校验
	//uint32_t crc;//整体crc校验
	uint32_t firmware_size;
		
}boot_info_t;

void BootInfo_Load(boot_info_t* info);
void BootInfo_Save(boot_info_t *info);
bool BootInfo_Valid(boot_info_t* info);
void BootInfo_InitDefault(boot_info_t* boot);
uint32_t Boot_GetAppAddr(app_id_t id);
uint32_t Boot_GetFirmwareSize(boot_info_t* info);
bool Bootloader_CRC32_Verify(uint32_t appAddr, uint32_t expected_crc,uint32_t firmware_size);
uint32_t Bootloader_GetCRC(app_id_t id,boot_info_t* info);
void Bootloader_Rollback(boot_info_t* info);
void Bootloader_SetPending(uint32_t pending_app,uint32_t firmware_size);
void Bootloader_MarkValid(void);
void bootloder_JumpToApp(uint32_t appAddr);


#endif
