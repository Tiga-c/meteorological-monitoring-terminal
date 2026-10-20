#include "LoRa.h"
#include "ESP_01S.h"
#include "stm32f4xx.h" 
#include "sys.h"
#define LORA_AUX_READ()   GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)
extern uint8_t wifi_is_ok;

void UART2_Init(uint32_t BaudRate)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	//1、使能时钟总线
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
	
	//2、选择引脚的复用功能为UART3
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource2 , GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource3 , GPIO_AF_USART2);
	
	//3、配置并初始化引脚
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 |  GPIO_Pin_3;	//配置引脚
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;				//工作模式：复用模式
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;			//频率：高速100MHZ
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;				//输出模式：推挽
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			//上下拉：浮空
	GPIO_Init(GPIOA , &GPIO_InitStruct);
	
	//4、配置并初始化UART3
	USART_InitStruct.USART_BaudRate = BaudRate;									//波特率
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;					//数据位长度：8bit
	USART_InitStruct.USART_StopBits = USART_StopBits_1;							//停止位
	USART_InitStruct.USART_Parity = USART_Parity_No;							//校验方式：无校验
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;				//工作模式：发送+接收
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控：无
	USART_Init(USART2 , &USART_InitStruct);
	
	//5、配置NVIC
	NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;			//中断向量/通道
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;	//抢占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;			//响应优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	//6、使能接收中断
	USART_ITConfig(USART2 , USART_IT_RXNE , ENABLE);
	
	//7、启动UART3
	USART_Cmd(USART2 , ENABLE);
}

void LoRa_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC , ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;	
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;   //内部上拉
	GPIO_Init(GPIOC , &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;	
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
//	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;	
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;	
	GPIO_Init(GPIOB , &GPIO_InitStruct);
	
	GPIO_SetBits(GPIOC, GPIO_Pin_0);	//PE9=H，WIFI上电运行
	
}

char usart2_buf[BUF_SIZE2] , usart2_buf[BUF_SIZE2];	//串口1接收并保存数据的缓冲区
volatile bool usart2_state , usart2_state;			//串口1的接收结束标志
volatile u8 usart2_count , usart2_count;			

void USART2_IRQHandler(void)
{
	u8 data;
	if(USART_GetITStatus(USART2 , USART_IT_RXNE))//判断接收中断标志位
	{		
		data = USART_ReceiveData(USART2);//接收数据
		
		if(data == '\r' || data == '\n')//消息接结束标志
			usart2_state = true;
		else
		{
			usart2_buf[usart2_count++] = data;
			if(usart2_count >= BUF_SIZE2)
			{
				usart2_count  = 0; 
				memset((char *)usart2_buf , 0, BUF_SIZE2);
			}
		}
		
		USART_ClearITPendingBit(USART2 , USART_IT_RXNE);//清楚中断标志位
	}
}

char LoRa_buf[32]; 		//上报字符串缓冲区，加大容量存多个传感器数据
char LoRa_buf1[32];
char LoRa_buf2[32];
char LoRa_buf3[32];
char LoRa_buf4[32];
char LoRa_buf5[32];
char LoRa_buf6[32];

uint16_t wait_cnt = 0;
uint8_t lora_is_ok = 0;
//判断WIFI是否正常，若WIFI连接失败，则启动LoRa传输
void LoRa_Send(float PCB_wet, float temp, u8 Humidity, float uv_power,uint32_t LightData_Hex, float alt, float press)
{
	
	if(wifi_is_ok)
		return;
	else
	{
//		while(LORA_AUX_READ() == 0)
//		{
//			delay_ms(1);
//			wait_cnt++;
//			if(wait_cnt > 5000)
//			{
//					//超时，判定本地LoRa模块异常
//					lora_is_ok = 0;
//					printf("lora开启失败！");
//					return;
//			}
//		}
//		lora_is_ok = 1;	

		
		GPIO_ResetBits(GPIOC,GPIO_Pin_0);
		delay_ms(30);
		
		snprintf(LoRa_buf, sizeof(LoRa_buf), "temp:%.2f\r\n", (temp/100)-4);
    printf("【LoRa上报温度】%s\r\n", LoRa_buf);
		USART_Send_msg(USART2,LoRa_buf);
    delay_ms(100);

    snprintf(LoRa_buf1, sizeof(LoRa_buf1), "humi:%d\r\n", Humidity);
    printf("【LoRa上报湿度】%s\r\n", LoRa_buf1);
    USART_Send_msg(USART2,LoRa_buf1);
    delay_ms(100);

    snprintf(LoRa_buf2, sizeof(LoRa_buf2), "ultraviolet:%.2f\r\n", uv_power);
    printf("【LoRa上报紫外线强度】%s\r\n", LoRa_buf2);
    USART_Send_msg(USART2,LoRa_buf2);;
    delay_ms(100);

    snprintf(LoRa_buf3, sizeof(LoRa_buf3), "moisture:%.2f\r\n", PCB_wet);
    printf("【LoRa上报湿润度】%s\r\n", LoRa_buf3);
    USART_Send_msg(USART2,LoRa_buf3);
    delay_ms(100);

    snprintf(LoRa_buf4, sizeof(LoRa_buf4), "illuminance:%d\r\n", (int)LightData_Hex);
    printf("【LoRa上报光照强度】%s\r\n", LoRa_buf4);
    USART_Send_msg(USART2,LoRa_buf4);
    delay_ms(100);

		snprintf(LoRa_buf5, sizeof(LoRa_buf5), "altitude:%.2f\r\n", alt);
    printf("【LoRa上报海拔】%s\r\n", LoRa_buf5);
    USART_Send_msg(USART2,LoRa_buf5);
    delay_ms(100);

		snprintf(LoRa_buf6, sizeof(LoRa_buf6), "pressure:%.2f\r\n", press/100);
    printf("【LoRa上报大气压强】%s\r\n", LoRa_buf6);
    USART_Send_msg(USART2,LoRa_buf6);
	}
}


