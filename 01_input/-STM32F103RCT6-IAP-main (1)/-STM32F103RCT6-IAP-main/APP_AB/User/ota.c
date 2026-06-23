#include "ota.h"
#include "ring_buffer.h"
#include "flash.h"
#include <string.h>
#include "crc_verify.h"
#include "bootloader.h"
#include "led.h"
#include <stdio.h>

//静态全局上下文变量，模块内维护和私有
static ota_ctx_t ctx;
//static parser_ctx_t parser;
//static handshake_frame_t hs;
//static data_frame_t data;

#define START_ADDRESS 0x08020800//APP_B的起始地址


static void ota_ctx_init(void)
{
	memset(&ctx,0,sizeof(ota_ctx_t));
	ctx.state=S_OTA_STATE_WAITHS;//空闲状态，等待握手帧
}

void ota_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma)
{
	app_protocol_init(huart,dma);
	//初始化上下文
	ota_ctx_init();
	
}


//OTA模块状态机任务切片
void ota_fsm_process(void)
{
	
	//轮询环形缓冲区（解析器里面也有检查缓冲区的逻辑）
	//if(ota_cache_is_empty()==true) return;//环形缓冲区为空，提前跳出
	switch(ctx.state)
	{
		case S_OTA_STATE_WAITHS:
		{
			if(ota_cache_is_empty()==true) return;//环形缓冲区为空，提前跳出
			
			//环形缓冲区非空:开始处理握手帧
			parser_err_t ret =app_parser_hs(&ctx.parser.output.hs,&ctx.parser);
			
			if(ctx.parser.frame_ready==true){
				 
				//握手帧解析完成，向上位机发送应答
				app_sendACK(FRAME_TYPE_ACK_HS,0);
				ctx.parser.frame_ready=false;
				//修改上下文
			ctx.state=S_OTA_STATE_WAITDATA;
			//ctx.crc=ctx.parser.crc;居然在这里把握手帧的CRC16赋值给OTA固件CRC32了！！！
				ctx.crc=ctx.parser.output.hs.firmware_crc;
			ctx.total_pkts=ctx.parser.output.hs.total_pkts;
			ctx.firmware=ctx.parser.output.hs.firmware_size;}
				//app_sendACK(FRAME_TYPE_NACK_HS,0);
			break;
		}
		
		case S_OTA_STATE_WAITDATA:
		{
		
			// 加这一行，确认进来了
			bool result=ota_cache_is_empty();
    //HAL_UART_Transmit(&huart1, (uint8_t*)"[W]\r\n", 5, 100);
			if(result==true) return;//环形缓冲区为空，提前跳出
			//缓冲区非空，处理数据帧
			app_parser_data(&ctx.parser.output.data,&ctx.parser);
			if(ctx.parser.frame_ready==true) {
				 
				//帧就绪，开始解析，对ready清零
				ctx.parser.frame_ready=false;
				ctx.seq=ctx.parser.seq;
				
				//将payload写入falsh
				if(ctx.seq % 4==0){
					//能被4整除再擦除一次，每次擦除1024=512*4
					flash_erase(START_ADDRESS+ctx.seq*PAYLOAD_MAX_LEN);
				}
				//记录要写入flash的字个数
				
				uint32_t word_count=(ctx.parser.len+3)/4;//向上取整
				//使用临时缓冲区存放写入flash的数据：对齐，防止访问payload越界
				static uint8_t aligned_buf[PAYLOAD_MAX_LEN + 3];
				memset(aligned_buf,0,sizeof(aligned_buf));
				memcpy(aligned_buf,ctx.parser.payload,ctx.parser.len);
				//写入falsh
				flash_write(START_ADDRESS+ctx.seq*PAYLOAD_MAX_LEN,(uint32_t*)aligned_buf,word_count);
				
				
				//flash写入后发送应答帧
				app_sendACK(FRAME_TYPE_ACK,ctx.seq);
				//写完以后判断序列号：序列号从零开始，配合flash擦除和写入
				if(ctx.seq==ctx.total_pkts-1)
				{
					ctx.state=S_OTA_STATE_VERIFY;//进入校验阶段
					break;//提前跳出循环
				}
				ctx.state=S_OTA_STATE_WAITDATA;//继续接收
			}
			break;
		}
		
		case S_OTA_STATE_VERIFY:
		{
			uint8_t* p = (uint8_t*)START_ADDRESS;
    
    
			#if 0
			char buf[120];
    // 1. 打印最后一个包的边界状况（看第 5135 字节后面的 Padding 是不是全为 0xFF）
    sprintf(buf, "\r\n[DEBUG]Boundary[5132..5139]: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
            p[5132], p[5133], p[5134], p[5135], p[5136], p[5137], p[5138], p[5139]);
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 200);
    
    // 2. 计算并打印两边的 CRC 期望值与实际值
    uint32_t length_words = (ctx.firmware + 3) / 4; //
    __HAL_CRC_DR_RESET(&hcrc); //
    uint32_t actual_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)START_ADDRESS, length_words); //
    
    sprintf(buf, "[DEBUG]CRC_RESULT: expect=0x%08lX actual=0x%08lX size=%lu\r\n", 
            ctx.crc, actual_crc, ctx.firmware); //
    HAL_UART_Transmit(&huart1, (uint8_t*)buf, strlen(buf), 200);
			#endif
    
			//进入校验阶段
			if(APP_Firmware_CRC32_Verify(START_ADDRESS,ctx.firmware,ctx.crc)==true)
			{
				//校验成功,写入bootinfo分区
				Bootloader_SetPending(APP_B,ctx.firmware);
				ctx.state=S_OTA_STATE_WAITHS;
				//点亮LED显示成功
				led1_on();
			}
			else
			{
				ctx.state=S_OTA_STATE_ERROR;
			}
			break;
		}
		
		case S_OTA_STATE_ERROR:
		{
		//失败了就闪灯
			led1_blinky(500);
		}
			
		default:
		break;
		
	}
	
}
