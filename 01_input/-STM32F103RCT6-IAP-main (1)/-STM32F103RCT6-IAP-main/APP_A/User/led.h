#ifndef LED_H
#define LED_H

#include <stdint.h>

void led1_on(void);
void led1_off(void);
void led2_on(void);
void led2_off(void);

//LED闪烁代码
void led1_blinky(uint32_t period);
void led2_blinky(uint32_t period);

#endif

