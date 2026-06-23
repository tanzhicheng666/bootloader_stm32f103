#include "bootloader.h"
#include "stm32f1xx_hal.h"
#include "flash.h"
#include <string.h>


#define BOOT_MAGIC 0x424F4F54

//BootInfo存储地址
#define BOOTINFO_ADDR 0x08008000
#define BOOTINFO ((boot_info_t*)BOOTINFO_ADDR)

//APP分区起始地址
#define APP_A_ADDR 0x08008800
#define APP_B_ADDR 0x08020800

extern CRC_HandleTypeDef hcrc; // 声明 HAL 库的硬件 CRC 句柄

#define APP_A_SIZE  (96 * 1024) // 96 KB
#define APP_B_SIZE  (96 * 1024) // 96 KB

typedef void (*pfunc)(void);//定义函数指针
static pfunc JumpToApp;




//读取BootInfo
void BootInfo_Load(boot_info_t* info)
{
	memcpy(info,BOOTINFO,sizeof(boot_info_t));
}

//修改bootinfo，写入flash
void BootInfo_Save(boot_info_t *info)
{
	//先擦除原来的flash
	flash_erase(BOOTINFO_ADDR);
	//写入心动bootinfo
	flash_write(BOOTINFO_ADDR,(uint32_t*)info,sizeof(boot_info_t)/4);
}

//判断是否合法
bool BootInfo_Valid(boot_info_t* info)
{
	return (info->magic==BOOT_MAGIC)?true:false;
}

//获取app地址
uint32_t Boot_GetAppAddr(app_id_t id)
{
	if(id==APP_B) return APP_B_ADDR;
	return APP_A_ADDR;
}



//初始化bootinfo
void BootInfo_InitDefault(boot_info_t* boot)
{
	boot->magic=BOOT_MAGIC;
	boot->active_app=APP_A;
	boot->pending_app=APP_NONE;
	boot->pre_app=APP_A;
	boot->state=OTA_STATE_NORMAL;//默认，分区A已经经过校验，是有效固件
	boot->boot_count=0;
	boot->app_a_crc=0;
	boot->app_b_crc=0;
	boot->firmware_size=0;
	
	BootInfo_Save(boot);
	
}



void bootloder_JumpToApp(uint32_t appAddr)
{
	 
		//#0. 【极其关键】关闭所有中断，关闭滴答定时器
    __disable_irq();
    SysTick->CTRL = 0; // 关闭滴答定时器，防止硬件继续计数触发中断
	//#1.读取MSP和ResetHandler入口地址
	uint32_t sp=*(uint32_t*)appAddr;
	uint32_t rst=*(uint32_t*)(appAddr+4);
	
	//#2.设置主堆栈指针MSP
	__set_MSP(sp);
	
	//#3.向量表重定位
	SCB->VTOR=appAddr;
	
	//#4.跳转到App的ResetHandler
	JumpToApp=(pfunc)rst;
	JumpToApp();//执行rst 
}




/**
 * @brief  计算并校验指定 APP 分区的固件完整性
 * @param  appAddr: APP分区的起始物理地址 (0x08008800 或 0x08020800)
 * @param  expected_crc: 从 boot_info_t 中读出来的期望 CRC32 值
 * @param  firmware_size: 从 boot_info_t 中读出来的新固件的真实大小值
 * @return bool: true 校验通过，false 校验失败
 */
bool Bootloader_CRC32_Verify(uint32_t appAddr, uint32_t expected_crc,uint32_t firmware_size)
{
    uint32_t calculated_crc = 0;
    uint32_t firmware_len_words = 0;
    
    // 1. 安全检查：判断该地址上有没有合法的有效固件（检查栈顶指针是否指向SRAM范围）
    // STM32F103RC 的 SRAM 范围是 0x20000000 到 0x2000C000 (48KB)
    uint32_t msp_val = *(volatile uint32_t*)appAddr;
    if((msp_val & 0x2FF00000) != 0x20000000)
    {
        return false; // 连合法的MSP栈顶都没有，绝对是空片或坏片，直接返回失败
    }

    // 2. 确定需要校验的字数（STM32硬件CRC是以32位/4字节为单位计算的）
    if (appAddr == APP_A_ADDR)
    {
        firmware_len_words = (firmware_size+3) / 4; // 96KB 转换为 24576 个字
    }
    else if (appAddr == APP_B_ADDR)
    {
        firmware_len_words = (firmware_size+3) / 4;
			//firmware_len_words = 2160;//先写死固件长度，测试成功，明天添加bootinfo的固件大小字段
    }
    else
    {
        return false; // 非法地址
    }

    // 3. 启动硬件 CRC 计算
    // HAL_CRC_Calculate 会直接通过总线读取 Flash 指定区域的内容并计算
    calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)appAddr, firmware_len_words);

    // 4. 【核心比对】：拿刚算出来的和结构体里的存盘值做比较
    if (calculated_crc == expected_crc)
    {
        return true;  // 完美的完整固件，放行！
    }
    else
    {
        return false; // 校验失败，固件损坏或不完整！
    }
}

void Bootloader_Rollback(boot_info_t* info)
{
		info->active_app=info->pre_app;
		info->boot_count=0;
		info->state=OTA_STATE_NORMAL;
}       
	

uint32_t Bootloader_GetCRC(app_id_t id,boot_info_t* info)
{
	if(id==APP_A)
	{
		return info->app_a_crc;
	}
	else
	{
		return info->app_b_crc;
	}
}

//获取新固件的大小，单位：byte，真实大小没有对齐
uint32_t Boot_GetFirmwareSize(boot_info_t* info)
{
	return info->firmware_size;
}

//提供给APP侧用于标记新固件Pending
void Bootloader_SetPending(uint32_t pending_app,uint32_t firmware_size)
{
	boot_info_t bt;
	BootInfo_Load(&bt);
	bt.pending_app=pending_app;
	bt.firmware_size=firmware_size;
	BootInfo_Save(&bt);
}

void Bootloader_MarkValid(void)
{
    boot_info_t bt;
    BootInfo_Load(&bt);
    if(bt.state==OTA_STATE_TESTING)
    {
        //修改bootinfo字段
        bt.state=OTA_STATE_NORMAL;
        bt.boot_count=0;
    }
    BootInfo_Save(&bt);
}


