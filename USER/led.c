#include "stm32f4xx.h"
#include "led.h"
void LED_init()
{
		GPIO_InitTypeDef myGPIO;
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
		myGPIO.GPIO_Pin = GPIO_Pin_9;
		myGPIO.GPIO_Mode = GPIO_Mode_OUT;
		myGPIO.GPIO_OType = GPIO_OType_PP;
		myGPIO.GPIO_Speed = GPIO_High_Speed;
		myGPIO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOF,&myGPIO);
		GPIO_ResetBits(GPIOF, GPIO_Pin_9);
	
		myGPIO.GPIO_Pin = GPIO_Pin_10;
		myGPIO.GPIO_Mode = GPIO_Mode_OUT;
		myGPIO.GPIO_OType = GPIO_OType_PP;
		myGPIO.GPIO_Speed = GPIO_High_Speed;
		myGPIO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOF,&myGPIO);
		GPIO_ResetBits(GPIOF, GPIO_Pin_10);
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
		myGPIO.GPIO_Pin = GPIO_Pin_13;
		myGPIO.GPIO_Mode = GPIO_Mode_OUT;
		myGPIO.GPIO_OType = GPIO_OType_PP;
		myGPIO.GPIO_Speed = GPIO_High_Speed;
		myGPIO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOE,&myGPIO);
		GPIO_ResetBits(GPIOE,GPIO_Pin_13);
		
		
		
		myGPIO.GPIO_Pin = GPIO_Pin_14;
		myGPIO.GPIO_Mode = GPIO_Mode_OUT;
		myGPIO.GPIO_OType = GPIO_OType_PP;
		myGPIO.GPIO_Speed = GPIO_High_Speed;
		myGPIO.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(GPIOE,&myGPIO);
		GPIO_ResetBits(GPIOE,GPIO_Pin_14);
	
		LED_AllOff();
			
}

void LED_AllOn()
{
		GPIO_ResetBits(GPIOE,GPIO_Pin_14);
		GPIO_ResetBits(GPIOE,GPIO_Pin_13);
		GPIO_ResetBits(GPIOF,GPIO_Pin_10);
		GPIO_ResetBits(GPIOF,GPIO_Pin_9);
}

void LED_AllOff()
{
		GPIO_SetBits(GPIOE,GPIO_Pin_14);
		GPIO_SetBits(GPIOE,GPIO_Pin_13);
		GPIO_SetBits(GPIOF,GPIO_Pin_10);
		GPIO_SetBits(GPIOF,GPIO_Pin_9);
}


