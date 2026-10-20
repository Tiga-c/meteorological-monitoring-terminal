#include "stm32f4xx.h"
#include "sys.h"
#include "delay.h"
#include "DHT11.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
//int fputc(int ch, FILE *f) {
//	// 添加串口数据发送代码
//	while((USART1->SR&0x40) == 0);//循环发送,直到发送完毕   
//    USART1->DR = (u8) ch; 
//	while((USART1->SR&0x80) == 0);
//	return ch;
//}

#define USART1_Recv_Len 10
__IO u8 USART1_Recv_buf[USART1_Recv_Len];
__IO u8 USART1_Recv_Flag=0;



//void USART1_IRQHandler(void)
//{
//	uint16_t data;
//	static u8 Recv_Status=0;
//	static u8 index=0;
//	
//	// 明确是何种中断类型
//	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
//	{
//		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//		
//		data = USART_ReceiveData(USART1);
//		switch(Recv_Status)
//		{
//			case 0:
//				if(data == 0xFF)
//				{
//					Recv_Status++;
//				}
//				break;
//			case 1:
//				USART1_Recv_buf[index++]=data;
//			    if(index>=1)  // 有效数据仅1字节
//				{
//					Recv_Status++;
//				}
//				if(index>=USART1_Recv_Len)
//				{
//					index=0;
//					Recv_Status=0;
//				}
//				break;
//			case 2:
//				if(data == 0xFE)
//				{
//					USART1_Recv_Flag=1;
//					index=0;
//					Recv_Status=0;
//				}
//			default:
//				break;
//		}	
//	}
//}



//void USART1_Config(uint32_t baud)
//{
//	GPIO_InitTypeDef GPIO_InitStruct;
//	USART_InitTypeDef USART_InitStruct;
//	NVIC_InitTypeDef NVIC_InitStruct;
//	
//	// 开启串口1的时钟
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
//	// 开启GPIOA的时钟
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
//	
//	// 配置GPIOA(9和10号)推挽复用功能
//	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
//	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
//	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_Init(GPIOA, &GPIO_InitStruct);
//	
//	// GPIO复用功能选择
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
//	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
//	
//	// 配置USART1硬件UART模式
//	USART_InitStruct.USART_BaudRate = baud;					 					 // 波特率：115200
//	USART_InitStruct.USART_WordLength = USART_WordLength_8b; 					 // 数据位：8位
//	USART_InitStruct.USART_StopBits = USART_StopBits_1;		 					 // 停止位：1位
//	USART_InitStruct.USART_Parity = USART_Parity_No;		 					 // 校验位：无
//	USART_InitStruct.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;					 // 串口功能：收/发
//	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 硬件流控：无
//	USART_Init(USART1, &USART_InitStruct);
//	
//	// 配置NVIC串口1中断通道
//	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
//	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
//	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
//	NVIC_Init(&NVIC_InitStruct);
//	
//	// 清除接收数据标志
//	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
//	// 开启接收中断
//	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
//	
//	// 清除发送完成标志
//	USART_ClearFlag(USART1, USART_FLAG_TC);
//	
//	// 开启硬件工作
//	USART_Cmd(USART1, ENABLE);
//}


void DHT11_OUT(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	

	// 开启GPIOA的时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	
	// 配置GPIOA(9和10号)推挽复用功能
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_Init(GPIOG, &GPIO_InitStruct);
	
	PGout(9) = 1;
	delay_ms(10);
}
void DHT11_IN(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	// 配置GPIOA(9和10号)推挽复用功能
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(GPIOG, &GPIO_InitStruct);
}

int GET_DHT11_Data(u8 *Humidity, u8 *Temperature)
{
	u8 data[5]={0};
	int t=0;
	// PG9输出起始信号
	DHT11_OUT();
	PGout(9) = 0;
	delay_ms(20);
	PGout(9) = 1;
	delay_us(25);
	
	// PG9输入读取模快的响应信号
	DHT11_IN();
	
	// 等待响应信号到来
	while(1)
	{
		t++;
		delay_us(10);
		if(PGin(9) == 0)
			break;
		if(t==10)
			return -1;
	}
	t=0;
	while(1)
	{
		t++;
		delay_us(10);
		if(PGin(9) != 0)
			break;
		if(t==10)
			return -2;
	}
	t=0;
	while(1)
	{
		t++;
		delay_us(10);
		if(PGin(9) == 0)
			break;
		if(t==10)
			return -1;
	}
	
	// 读取数据
	for(int i=0; i<5; i++)
	{
		for(int j=0; j<8; j++)
		{
			// 每一bit都有一个50us的低电平
			while(PGin(9)==0);
			// bit0（26-28us）与bit1（70us）的区别是高电平的持续时间
			delay_us(30);
			if(PGin(9))
			{
				data[i] |= 0x01<<(7-j);
				while(PGin(9)); // bit1后的高电平持续完毕
			}
		}
	}
	DHT11_OUT();
	
	// 校验数据
	if(data[4] == data[0]+data[1]+data[2]+data[3])
	{
		*Humidity = data[0];
		*Temperature = data[2];
	}
	
	return 0;
}



