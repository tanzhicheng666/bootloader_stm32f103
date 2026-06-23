#ifndef OTA_H
#define OTA_H

#include "protocol.h"

typedef enum {

	S_OTA_STATE_WAITHS,	//没有OTA请求
	S_OTA_STATE_WAITDATA,//等待数据帧状态
	S_OTA_STATE_VERIFY,//CRC校验阶段
	S_OTA_STATE_ERROR,	//OTA出错
}s_ota_state_t;

typedef struct {

	s_ota_state_t state;	//OTA状态
	parser_ctx_t parser;//解析器上下文
	uint16_t total_pkts;//固件总包数
	uint32_t firmware;//固件大小
	uint32_t crc;//接收到固件CRC校验码
	uint16_t seq;//固件包序列号
	
	

} ota_ctx_t;

void ota_fsm_process(void);//状态机任务切片
void ota_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma);//OTA模块初始化

#endif

