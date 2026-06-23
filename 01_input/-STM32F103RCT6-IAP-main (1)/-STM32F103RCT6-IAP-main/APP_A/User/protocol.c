#include "protocol.h"
#include "crc_verify.h"
#include "transport.h"
#include "ring_buffer.h"
#include <string.h>
#include <stdio.h>


#define ACK_SOF 0xA6
#define DATA_SOF 0xA5
#define HANDSHAKE_SOF 0xA5

#define HANDSHAKE_FRAME_LEN 14	//握手帧长度

static parser_ctx_t s_ctx;//解析器上下文变量
static ring_buffer_t* s_rb;


//获取解析器上下文指针
parser_ctx_t* get_parser_ctx(void)
{
	return &s_ctx;
}

ring_buffer_t* get_ota_cache(void)
{
	return s_rb;
}

//向上层提供查询环形缓冲区是否未空接口
bool ota_cache_is_empty(void)
{
	return rb_is_empty(s_rb);
}

void app_protocol_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma)
{
	trans_init(huart,dma);
	s_rb=trans_get_rb();//获取环形缓冲区指针，用于解析数据
}

#if 1
void app_sendACK(frame_type_t type,uint16_t seq)
{
	//拼帧
	//HAL_UART_Transmit(&huart1,(uint8_t*)"enter app send",14,1000);
	//trans_send((uint8_t*)"enter app send",14);
	ack_frame_t frame;
	frame.sof=0xA6;
	//MCU侧小端序，上位机大端序
	frame.seq=(uint16_t)((seq<<8)|(seq>>8));
	frame.type=type;
	frame.crc=APP_Compute_CRC16((uint8_t*)&frame,(sizeof(ack_frame_t)-2));
	trans_send((uint8_t*)&frame,sizeof(ack_frame_t));
}
#endif

#if 0
void app_sendACK(frame_type_t type, uint16_t seq)
{
    ack_frame_t frame;
    frame.sof = 0xA6;
    frame.seq = (seq >> 8) | (seq << 8);
    frame.type = type;
    frame.crc = APP_Compute_CRC16((uint8_t*)&frame, sizeof(ack_frame_t)-2);
    
    // 调试打印
    char dbg[64];
    sprintf(dbg, "ACK: seq_in=%d, seq_out=0x%04X\r\n", seq, frame.seq);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);
    
    trans_send((uint8_t*)&frame, sizeof(ack_frame_t));
}
#endif

#if 0
void app_sendACK(frame_type_t type, uint16_t seq)
{
    ack_frame_t frame;
    frame.sof = 0xA6;
    frame.seq = (seq >> 8) | (seq << 8);  // 修正后的转换
    frame.type = type;
    frame.crc = APP_Compute_CRC16((uint8_t*)&frame, sizeof(ack_frame_t)-2);
    
    // 调试：打印整个帧
    char dbg[128];
    sprintf(dbg, "ACK: seq_in=%d, seq_out=%d (0x%04X)\r\n", seq, frame.seq, frame.seq);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);
    
    // 打印原始字节（十六进制）
    uint8_t hex_dbg[64];
    sprintf((char*)hex_dbg, "Frame HEX: ");
    HAL_UART_Transmit(&huart1, hex_dbg, strlen((char*)hex_dbg), 100);
    for(int i=0; i<sizeof(ack_frame_t); i++) {
        char byte_str[8];
        sprintf(byte_str, "%02X ", ((uint8_t*)&frame)[i]);
        HAL_UART_Transmit(&huart1, (uint8_t*)byte_str, strlen(byte_str), 100);
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
    
    trans_send((uint8_t*)&frame, sizeof(ack_frame_t));
}
#endif

//握手帧解析器:握手帧长度固定，不使用状态机
parser_err_t app_parser_hs(handshake_frame_t* hs,parser_ctx_t* ctx)
 {
	 
	 // 【强制自救一行】不管之前 s_rb 经历了什么，这里重新去运输层拿一次真的地址
    //s_rb = trans_get_rb();
	//顾虑掉可能残留的字节，先确认SOF字节
	while(!rb_is_empty(s_rb))
	{
		//环形缓冲区非空
		uint8_t peek;
		rb_peek(s_rb,&peek);
		if(peek==HANDSHAKE_SOF) break;//读到握手帧SOF，跳出循环
		//不是SOF，丢掉
		rb_get(s_rb,&peek);
	}
	
	//握手帧长度固定，等待到齐了再解析
	if(rb_size(s_rb)<sizeof(handshake_frame_t)) return PARSER_ERR_NRX;//不够就返回
	//先获取前12个字节（不含握手帧CRC16校验）
	uint8_t hs_raw[12];
	rb_peeks(s_rb,hs_raw,sizeof(hs_raw));
	//接收完成开始读取
	rb_get(s_rb,&hs->sof);
	rb_get(s_rb,&hs->type);
	
	//固件大小：uint32_t类型=4个字节
	uint8_t fm_size[4];
	rb_read(s_rb,fm_size,4);
	hs->firmware_size=(fm_size[0]<<8*3)|(fm_size[1]<<8*2)|(fm_size[2]<<8*1)|fm_size[3];
	//总包数 ：uint16_t类型=2个字节
	uint8_t pkt[2];
	rb_read(s_rb,pkt,2);
	hs->total_pkts=(pkt[0]<<8)|pkt[1];
	//固件CRC32：uint32_t类型=4个字节
	uint8_t fm_crc[4];
	rb_read(s_rb,fm_crc,4);
	hs->firmware_crc=(fm_crc[0]<<8*3)|(fm_crc[1]<<8*2)|(fm_crc[2]<<8*1)|fm_crc[3];
	//CRC16校验 ：uint16_t类型=2个字节
	uint8_t crc[2];
	rb_read(s_rb,crc,2);
	uint16_t except=(crc[0]<<8)|crc[1];
	if(APP_CRC16_Verify(hs_raw,12,except)!=true) 
	{
		//16位CRC校验失败，发送NACK
		app_sendACK(FRAME_TYPE_NACK_HS,0);
		return PARSER_ERR_CRC16;
	}
	hs->crc=except;
	//写入ctx
	ctx->type=hs->type;
	ctx->crc=hs->crc;
	ctx->frame_ready=true;//握手帧就绪
	//memcpy(&ctx.output.hs,hs,sizeof(handshake_frame_t));//可以把上下文结构体成员通过指针传进来
	return PARSER_OK;
}

//解析器状态机，只处理packet数据包，不管握手帧(只是上层OTA状态机的一个小分支)
void app_parser_data(data_frame_t* frame,parser_ctx_t* ctx)
{
	switch(ctx->state)
	{
		case PARSER_WAIT_SOF:
		{
			//尝试从环形缓冲区读取数据
			uint8_t sof;
			if(rb_is_empty(s_rb)==true) return;//环形缓冲区为空，退出
			rb_get(s_rb,&sof);
			if(sof==0xA5)
			{
				ctx->state=PARSER_WAIT_TYPE;
			}
			break;
			
		}
		
		case PARSER_WAIT_TYPE:
		{
			//type字段只占一个字节
			if(rb_is_empty(s_rb)==true) return;//环形缓冲区为空，退出
			rb_get(s_rb,&ctx->type);
			if(ctx->type==FRAME_TYPE_PKT)
			{
				ctx->state=PARSER_WAIT_SEQ_LEN;//确认是数据帧，等待序列号和payload长度
			}
			break;
		}
		
		case PARSER_WAIT_SEQ_LEN:
		{
			//两个字段，一共4个字节
			if(rb_size(s_rb)>=4)
			{
				//读取四个字节
				//上位机：大端模式：高字节在低地址（前） 小端模式：低字节在低地址（前）
				uint8_t byte[4];
				//从环形缓冲区里面读取序列和长度信息
				rb_read(s_rb,byte,4);
				ctx->seq=(byte[0]<<8)|byte[1];
				ctx->len=(byte[2]<<8)|byte[3];
				
				//len合法性检查
				if(ctx->len>PAYLOAD_MAX_LEN)
				{
					//超出最大边界，无效丢弃
					ctx->state=PARSER_WAIT_SOF;
				}
				else
				{
					//合法len,进入下一个状态
					ctx->state=PARSER_WAIT_PKT;
				}
			}
			break;
		}
		
		case PARSER_WAIT_PKT:
		{
			if(rb_size(s_rb)>=ctx->len)
			{
				//payload完整写入rb,从rb读出放入ctx上下文
				rb_read(s_rb,ctx->payload,ctx->len);
				//读取payload完成进入下一个state
				ctx->state=PARSER_WAIT_CRC;
				
			}
			break;
		}
		
		case PARSER_WAIT_CRC:
		{
			if(rb_size(s_rb)>=2)
			{
				uint8_t veri[2];
				rb_read(s_rb,veri,2);
				ctx->crc=(veri[0]<<8)|veri[1];
				if(APP_Ex_CRC16_Verify(ctx->payload,ctx->len,ctx->seq,ctx->crc)==true)
				{
					//CRC校验成功，回到空闲状态
					ctx->frame_ready=true;
					ctx->state=PARSER_WAIT_SOF;
					
					//填充output frame
					frame->sof=0xA5;
					frame->type=ctx->type;
					frame->seq=ctx->seq;
					frame->len=ctx->len;
					memcpy(frame->payload, ctx->payload, ctx->len);
					frame->crc=ctx->crc;
					
				}
				else{
				ctx->state=PARSER_STATE_ERROR;//CRC校验失败，进入错误状态
				}
			}
			break;
		}
		
		case PARSER_STATE_ERROR:
		{
			//CRC错误，NACK,重新等待SOF
			app_sendACK(FRAME_TYPE_NACK,ctx->seq);
			ctx->state=PARSER_WAIT_SOF;
			
			break;
		}
		
		default:
			break;
	
	}

}
