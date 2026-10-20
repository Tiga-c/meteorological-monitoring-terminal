#include "TIMER.h"
#include "ESP_01S.h"

//初始化TIM3
void TIM3_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	//1、使能TIM3的时钟总线
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/*
	2、配置并初始化TIM3
	APB1 ： 42MHZ
	TIM3 = APB1*2 = 84MHZ
	TIM3 / 8400 = 10000HZ
	计数次数：10000
	T = 1s
	*/
	TIM_TimeBaseInitStruct.TIM_Prescaler = 8400-1;	//预分频
	TIM_TimeBaseInitStruct.TIM_Period = 10000-1;		//自动重装载值
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;	//计数模式：向上计数模式
	//TIM_TimeBaseInitStruct.TIM_RepetitionCounter = ;
	//TIM_TimeBaseInitStruct.TIM_ClockDivision = ;//时钟分频
	TIM_TimeBaseInit(TIM3 , &TIM_TimeBaseInitStruct);
	
	//3、配置NVIC
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//4、使能定时器中断
	TIM_ITConfig(TIM3 , TIM_IT_Update ,ENABLE);
	//5、启动TIM3
	TIM_Cmd(TIM3 , ENABLE);
}

//中断服务函数--TIM3定时结束
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3 , TIM_IT_Update))//判断中带你标志位
	{
		heartbeat();
		TIM_ClearITPendingBit(TIM3 , TIM_IT_Update);//清除中断标志位
	}
}
