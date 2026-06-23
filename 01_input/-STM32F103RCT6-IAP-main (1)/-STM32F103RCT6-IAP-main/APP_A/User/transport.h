#ifndef TRANSPORT_H
#define TRANSPORT_H


#include "stm32f1xx_hal.h"
#include "ring_buffer.h"






void trans_init(UART_HandleTypeDef* huart,DMA_HandleTypeDef* dma);
void trans_send(uint8_t* data,uint16_t len);
ring_buffer_t* trans_get_rb(void);

#endif
