#ifndef __LoRa_H__
#define __LoRa_H__
#include "stm32f4xx.h" 
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>
#define BUF_SIZE2 256



void UART2_Init(uint32_t BaudRate);
void USART2_IRQHandler(void);
void USART2_Send_msg(USART_TypeDef* USARTx , char * msg );
void LoRa_Send(float PCB_wet, float temp, u8 Humidity, float uv_power,uint32_t LightData_Hex, float alt, float press);
void LoRa_GPIO_Init(void);
#endif

