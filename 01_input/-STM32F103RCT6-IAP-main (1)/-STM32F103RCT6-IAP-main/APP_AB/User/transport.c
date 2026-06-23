#include "transport.h"

#include "usart.h"
#include <string.h>
#include "led.h"




//缓冲区长度设置
#define RX_BUF_LEN 1024
#define CACHE_RB_LEN 1024


static UART_HandleTypeDef* _huart;
static DMA_HandleTypeDef* _dma;
static uint8_t _rx_buf[RX_BUF_LEN];//临时接收缓冲区
static ring_buffer_t cache;
static rb_elem_t rb_array[CACHE_RB_LEN];

void trans_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma)
{
	_huart=huart;
	_dma=dma;
	//初始化环形缓冲区
	rb_init(&cache,rb_array,CACHE_RB_LEN);
	//开启接收空闲中断,关闭DMA过半中断
	HAL_UARTEx_ReceiveToIdle_DMA(_huart,_rx_buf,RX_BUF_LEN);
	__HAL_DMA_DISABLE_IT(_dma,DMA_IT_HT);
}

//使用空闲中断进行接收
//@参数：size：接收到的字节数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
 if(huart==_huart)
 {
	 led1_off();
	 //将一帧数据包写入环形缓冲区
	 rb_write(&cache,_rx_buf,Size);
	 //测试代码
	 //char dt[20]="aaaaa";
	 //HAL_UART_Transmit(huart,dt,strlen(dt),1000);
	 //HAL_UART_Transmit(huart,_rx_buf,Size,1000);
	 // 【关键调试步骤】抓取启动返回值
    HAL_StatusTypeDef status;
    status = HAL_UARTEx_ReceiveToIdle_DMA(_huart, _rx_buf, RX_BUF_LEN);
    
    if(status != HAL_OK)
    {
        // 🚨 请在这里打一个断点！！
        // 如果程序一启动就停在这行，说明开启接收失败了
        __NOP(); 
    }
    
    __HAL_DMA_DISABLE_IT(_dma, DMA_IT_HT);
 }
}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if(huart == _huart)
//    {
//        // 在这里打断点！！
//        HAL_UART_Receive_IT(_huart, _rx_buf, 1);
//    }
//}

//传输层发送函数
void trans_send(uint8_t* data,uint16_t len)
{
	HAL_UART_Transmit(_huart,data,len,1000);
}

//传输层返回环形缓冲区的地址
ring_buffer_t* trans_get_rb(void)
{
	return &cache;
}


