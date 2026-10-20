#include "delay.h"


/*void delay_ms(uint32_t ms)
{
	
		SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); //div8(除以8) 8分频
		while(ms --)
		{
			SysTick->CTRL = 0; 			// 关闭系统定时器后才能配置寄存器
			SysTick->LOAD = 21000; 			// 设置计数值，用于设置定时的时间
			SysTick->VAL = 0; 			// 清空当前值还有计数标志位
			SysTick->CTRL = 1; 			// 使能系统定时器工作，且时钟源为系统时钟的8分频（168MHz/8=21MHz）
															//并开启定时器
			while ((SysTick->CTRL & (1<<16))==0);	// 等待系统定时器计数完毕
			SysTick->CTRL = 0; 			// 关闭系统定时器	
		}

	}*/
//u32 sys_tick_ms = 0;
//			void SysTick_Init(void)
//	{
//			// SystemCoreClock：F4默认168MHz， 168M / 1000 = 1ms中断一次
//			SysTick_Config(SystemCoreClock / 1000);
//	}

//	
//	void delay(u32 nms)
//	{
//			u32 start = sys_tick_ms;
//			while(sys_tick_ms - start < nms);
//	

//}
	
void delay_init(void)
{
	// 设置系统定时器时钟源为HCLK（168MHz）
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

void delay_us(int nus)
{
	SysTick->CTRL = 0; // Disable SysTick 关闭系统定时器
	SysTick->LOAD = 168*nus-1; // 装置值：从这个值递减0
	SysTick->VAL = 0; // Clear current value as well as count flag
	SysTick->CTRL = 5; // Enable SysTick timer with processor clock
	while ((SysTick->CTRL & 0x00010000)==0);// Wait until count flag is set
	SysTick->CTRL = 0; // Disable SysTick
}

void delay_ms(int nms)
{
	while(nms--)
	{
		delay_us(1000); // 当前配置下最大定时时间是99ms
	}
}
