#include "led.h"
#include "gpio.h"
//#include "delay.h"

//静态全局变量
//static uint32_t last_tick=0;//当前tick值

void led1_on(void)
{
	HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_SET);
}

void led1_off(void)
{
	HAL_GPIO_WritePin(LED1_GPIO_Port,LED1_Pin,GPIO_PIN_RESET);
}

void led2_on(void)
{
	HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,GPIO_PIN_SET);
}

void led2_off(void)
{
	HAL_GPIO_WritePin(LED2_GPIO_Port,LED2_Pin,GPIO_PIN_RESET);
}
	

void led1_blinky(uint32_t period)
{
	led1_on();
	HAL_Delay(period);
	led1_off();
	HAL_Delay(period);
}

void led2_blinky(uint32_t period)
{
	led2_on();
	HAL_Delay(period);
	led2_off();
	HAL_Delay(period);
}


