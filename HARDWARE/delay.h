#ifndef __DELAY_H
#define __DELAY_H
#include "stm32f4xx.h"
//void delay_ms(uint32_t ms);
//void SysTick_Init(void);
//void delay(u32 nms);
void delay_init(void);
void delay_ms(int nms);
void delay_us(int nus);
#endif
