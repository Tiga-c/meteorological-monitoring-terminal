#include "stm32f4xx.h"
#include "key.h"
#include "delay.h"




void KEY_Init(void)
{
		GPIO_InitTypeDef myGPIO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

		//PA0 按键
		myGPIO.GPIO_Pin = GPIO_Pin_0;
		myGPIO.GPIO_Mode = GPIO_Mode_IN;
		myGPIO.GPIO_OType = GPIO_OType_PP;    
		myGPIO.GPIO_Speed = GPIO_High_Speed;
		myGPIO.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOA, &myGPIO);

		//PE0 PE1 PE2 三个按键
		myGPIO.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
		myGPIO.GPIO_Mode = GPIO_Mode_IN;
		myGPIO.GPIO_OType = GPIO_OType_PP;    
		myGPIO.GPIO_Speed = GPIO_High_Speed;  
		myGPIO.GPIO_PuPd = GPIO_PuPd_UP;
		GPIO_Init(GPIOE, &myGPIO);
}

uint8_t KEY_scan()
{
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0)
			{
					delay_ms(20);
					if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0)
					{
						while(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 0);
						delay_ms(20);
						return 1;
					}
			}
			
			
			
			if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 0)
			{
					delay_ms(20);
					if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 0)
					{
						while(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_2) == 0);
						delay_ms(20);
						return 2;
					}
			}
			
			
			if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3) == 0)
			{
					delay_ms(20);
					if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3) == 0)
					{
						while(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_3) == 0);
						delay_ms(20);
						return 3;
					}
			}
			
			
			
			
			if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4) == 0)
			{
					delay_ms(20);
					if(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4) == 0)
					{
						while(GPIO_ReadInputDataBit(GPIOE,GPIO_Pin_4) == 0);
						delay_ms(20);
						return 4;
					}
			}
			return 0;
}
