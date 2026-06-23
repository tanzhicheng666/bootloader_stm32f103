#ifndef PROTOCOL_H
#define PROTCCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"

#define PAYLOAD_MAX_LEN 512


#define FRAME_TYPE_HANDSHAKE    0x10
#define FRAME_TYPE_ACK_HS				0x11//握手帧应答帧类型字段
#define FRAME_TYPE_NACK_HS			0x12
#define FRAME_TYPE_PKT          0x20//数据帧应答帧
#define FRAME_TYPE_ACK          0x80
#define FRAME_TYPE_NACK         0x81

typedef uint8_t frame_type_t;

typedef struct handshake_frame_t{
	uint8_t sof;//数据帧包头：0x55
	frame_type_t type;//数据帧类型
	uint32_t firmware_size;//固件大小
	uint16_t total_pkts;//总包数
	uint32_t firmware_crc;//固件二进制文件CRC32
	uint16_t crc;//帧校验
}__attribute__((packed))handshake_frame_t;

typedef struct data_frame_t {
	uint8_t sof;
	frame_type_t type;
	uint16_t seq;//数据包序列号
	uint16_t len;//payload长度
	uint8_t payload[PAYLOAD_MAX_LEN];
	uint16_t crc;//数据帧CRC16校验

}__attribute__((packed)) data_frame_t;

typedef struct ack_frame_t {

	uint8_t sof;//0xA6
	frame_type_t type;//0x00:NACK	0x01：ACK
	uint16_t seq;//对应数据包的序列号
	uint16_t crc;//响应帧的CRC16校验码
}__attribute__((packed)) ack_frame_t;

//数据帧解析状态
typedef enum parser_state_t{
	PARSER_WAIT_SOF,
	PARSER_WAIT_TYPE,
	PARSER_WAIT_SEQ_LEN,
	PARSER_WAIT_PKT,
	PARSER_WAIT_CRC,
	PARSER_STATE_ERROR,
}parser_state_t;

//解析器错误码
typedef enum {

	PARSER_ERR_CRC16,//16位CRC校验失败
	PARSER_ERR_NRX,//帧接收不完整，环形缓冲区没有写完
	PARSER_OK,//OK/操作成功
}parser_err_t;




typedef struct paraser_ctx_t{
	
	parser_state_t state;//只保留给数据帧解析使用
	
	frame_type_t type;
	uint16_t seq;
	uint16_t len;
	uint8_t payload[512];
	uint16_t crc;
	bool frame_ready;
	union {
		
		data_frame_t data;
		handshake_frame_t hs;
	}output;//输出帧格式：数据帧/握手帧
	
}parser_ctx_t;

void app_protocol_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma);
void app_sendACK(frame_type_t type,uint16_t seq);
void app_parser_data(data_frame_t* frame,parser_ctx_t* ctx);
parser_err_t app_parser_hs(handshake_frame_t* hs,parser_ctx_t* ctx);
parser_ctx_t* get_parser_ctx(void);
bool ota_cache_is_empty(void);


#endif
