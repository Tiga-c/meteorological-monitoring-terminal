#ifndef __DHT11_H
#define __DHT11_H
#include "stm32f4xx.h"   
#include "stdio.h"


int fputc(int ch, FILE *f);
void USART1_IRQHandler(void);
void USART1_Config(uint32_t baud);
void DHT11_OUT(void);
void DHT11_IN(void);
int GET_DHT11_Data(u8 *Humidity, u8 *Temperature);


#endif
