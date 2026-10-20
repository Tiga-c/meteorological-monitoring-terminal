#include "ESP_01S.h"
#include "delay.h"
#include "sys.h"

char usart1_buf[BUF_SIZE] , usart3_buf[BUF_SIZE];	//串口1接收并保存数据的缓冲区
volatile bool usart1_state , usart3_state;			//串口1的接收结束标志
volatile u8 usart1_count , usart3_count;			//串口1接收数据量
u32 esp_data_len = 0;       // 解析后的数据长度
u8 *esp_data_ptr = NULL;    // 解析后的数据指针

u16 HeartBeat;



volatile u8 state = AT;

//发送AT指令
void ESP_O1S_send_AT(char * AT){
	USART_Send_msg(USART3 , AT);
}

/*
发送数据
*/
void ESP_O1S_send_msg(char * msg){
	ESP_O1S_send_AT(msg);
	HeartBeat = 0;
	delay_ms(50);
}



/*
发送命令
cmd :消息类型
	1	订阅消息：订阅主题，接收消息
	2	发布消息：向主题发送消息
	3	订阅+历史：订阅主题并拉取最新消息
	7	获取时间：获取服务器时间
	9	获取历史：拉取主题最新消息
topic：主题名称（设备标识）
msg：
	cmd==2 :消息内容
	cmd==7：type获取的时间类型
		type=1 获取当前日期和时间，例如：2021-06-11 17:20:54
		type=2 获取当前时间，例如：17:20:54
		type=3 获取当前时间戳，例如：1623403325
*/
void ESP_send_command(u8 cmd , char * topic  , char * msg){
	char buf[128]={0};
	
	switch(cmd)
	{
		case SUB:
		case SUB_HIS:
		case GET_HIS:{
			snprintf(buf ,127, "cmd=%d&uid=%s&topic=%s\r\n",cmd , UID,topic);
			break;
		}
		case PUB:{
			snprintf(buf ,127, "cmd=%d&uid=%s&topic=%s&msg=%s\r\n",
			cmd , UID,topic,msg);
			break;
		}
		case GET_TIME:{
			snprintf(buf ,127, "cmd=%d&uid=%s&type=%s\r\n",cmd , UID,msg);
			break;
		}
	}
	ESP_O1S_send_msg(buf);
}


/*
ESP-01S初始化：
设置为station模式，连接热点，进入透传，连接TCP服务器
*/


uint8_t wifi_is_ok = 0;
void ESP_01S_Link_AP_and_Server(void){
	char command[50];
	u8 step_retries = 0;
	const u8 max_retries = 3;
	ESP_O1S_send_AT("+++");
	delay_ms(1000);
	ESP_O1S_send_AT("AT+RST\r\n");
	delay_ms(1000);
	ESP_O1S_send_AT("AT\r\n");
	
	while(1){
		if(usart3_state){
			printf("%s\r\n" , (char *)usart3_buf);
			if(strstr((char *)usart3_buf , "OK") || strstr((char *)usart3_buf , "ERROR")){
				if(strstr((char *)usart3_buf , "OK")){
					step_retries = 0;
					state++;
					if(state == CIPSEND+1)
						break; //?全部AT流程完成，跳出while，代表连接成功
				}
				else if(strstr((char *)usart3_buf , "ERROR")){
					step_retries++;
                    if(step_retries >= max_retries)
                    {
                        printf("Error at state %d, 达到最大重试次数，WIFI连接失败！\r\n", state);
                        wifi_is_ok = 0;    //标志位置0
                        goto ESP_LOOP_EXIT;//??直接跳出while(1)，退出WIFI连接逻辑！
                    }
                    else
                    {
                        printf("Error at state %d, 重试中...\r\n", state);
                        
                        
                    }
				}
			
				memset((char *)command , 0, 50);
				switch(state){
					case AT : sprintf(command , "AT\r\n");break;
					case CWMODE : sprintf(command , "AT+CWMODE=3\r\n");break;
					case CWJAP : sprintf(command , "AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PWD);break;
					case CIFSR : sprintf(command , "AT+CIFSR\r\n");break;
					case CIPSTART : sprintf(command , "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n" , IP , PORT);break;
					case CIPMODE: sprintf(command , "AT+CIPMODE=1\r\n");break;
					case CIPSEND: sprintf(command , "AT+CIPSEND\r\n");break;					
				}
				ESP_O1S_send_AT(command);
			}
			
			usart3_state = false;
			usart3_count  = 0; 
			memset((char *)usart3_buf , 0, BUF_SIZE);
		} 
		delay_ms(100);
	}
ESP_LOOP_EXIT: //goto跳转到此位置
	//判断是正常跑完流程退出，还是失败goto跳出来
	if(state == CIPSEND+1)
	{
		printf("连接TCP 服务器 成功\r\n");
		wifi_is_ok = 1;
	}
	else
	{
		printf("=== WIFI/TCP上电连接失败，启用LoRa备份通道 ===\r\n");
		wifi_is_ok = 0;
		GPIO_ResetBits(GPIOB, GPIO_Pin_0);
	
	}
}









//uint8_t wifi_is_ok = 0;
//void ESP_01S_Link_AP_and_Server(void){
//	char command[50];
//	u8 step_retries = 0;
//	const u8 max_retries = 3;

//	ESP_O1S_send_AT("+++");
//	delay_ms(1000);
//	ESP_O1S_send_AT("AT+RST\r\n");
//	delay_ms(1000);
//	ESP_O1S_send_AT("AT\r\n");
//	
//	while(1){
//		if(usart3_state){//接收到模块返回的数据
//			//n=0;
//			printf("%s\r\n" , (char *)usart3_buf);
//			//一条指令的返回数据已经接收完毕
//			if(strstr((char *)usart3_buf , "OK") || strstr((char *)usart3_buf , "ERROR")){
//				if(strstr((char *)usart3_buf , "OK")){
//					state++;
//					if(state == CIPSEND+1)
//						break;
//				}
//				else if(strstr((char *)usart3_buf , "ERROR")){
//					step_retries++;
//                    if(step_retries >= max_retries)
//                    {
//                        printf("Error at state %d, retrying or exiting.\r\n", state);
//                        
//                        state = AT;
//                        step_retries = 0;
//                     
//                    }
//				}
//			
//				memset((char *)command , 0, 50);
//				switch(state){
//					case AT : sprintf(command , "AT\r\n");break;
//					case CWMODE : sprintf(command , "AT+CWMODE=3\r\n");break;
//					case CWJAP : sprintf(command , "AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PWD);break;
//					case CIFSR : sprintf(command , "AT+CIFSR\r\n");break;
//					case CIPSTART : sprintf(command , "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n" , IP , PORT);break;
//					case CIPMODE: sprintf(command , "AT+CIPMODE=1\r\n");break;
//					case CIPSEND: sprintf(command , "AT+CIPSEND\r\n");break;					
//				}
//				ESP_O1S_send_AT(command);
//			}
//			
//			
//			usart3_state = false;
//			usart3_count  = 0; 
//			memset((char *)usart3_buf , 0, BUF_SIZE);
//		} 
//		delay_ms(100);
//	}
//	printf("连接TCP 服务器 成功\r\n");
//}



//心跳包
void heartbeat(void){
	ESP_O1S_send_msg("ping\r\n");
	HeartBeat =0;
}

/*
判读获取到的是不是巴法云的服务器时间
2021-06-11 16:39:27
注：没有\r\n
*/
bool Is_server_time(void){
	int i ;
	for(i=0 ; i<4 ;i++)//2021
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	if(usart3_buf[i++] != '-')
		return false;
	for( ; i<7 ;i++)//06
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	if(usart3_buf[i++] != '-')
		return false;
	for( ; i<10 ;i++)//11
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	if(usart3_buf[i++] != ' ')
		return false;
	for( ; i<13 ;i++)//16
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	if(usart3_buf[i++] != ':')
		return false;
	for( ; i<16 ;i++)//16
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	if(usart3_buf[i++] != ':')
		return false;
	for( ; i<19 ;i++)//16
	{
		if(usart3_buf[i]< '0' || usart3_buf[i]> '9')
			return false;
	}
	
	return true;
}


/*
初始化UART3
RX:PB11
TX:PB10
*/
void UART3_Init(uint32_t BaudRate)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	//1、使能时钟总线
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB , ENABLE);
	
	//2、选择引脚的复用功能为UART3
	GPIO_PinAFConfig(GPIOB , GPIO_PinSource11 , GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB , GPIO_PinSource10 , GPIO_AF_USART3);
	
	//3、配置并初始化引脚
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11 |  GPIO_Pin_10;	//配置引脚
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;				//工作模式：复用模式
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;			//频率：高速100MHZ
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;				//输出模式：推挽
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			//上下拉：浮空
	GPIO_Init(GPIOB , &GPIO_InitStruct);
	
	//4、配置并初始化UART3
	USART_InitStruct.USART_BaudRate = BaudRate;									//波特率
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;					//数据位长度：8bit
	USART_InitStruct.USART_StopBits = USART_StopBits_1;							//停止位
	USART_InitStruct.USART_Parity = USART_Parity_No;							//校验方式：无校验
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;				//工作模式：发送+接收
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控：无
	USART_Init(USART3 , &USART_InitStruct);
	
	//5、配置NVIC
	NVIC_InitStruct.NVIC_IRQChannel = USART3_IRQn;			//中断向量/通道
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;	//抢占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;			//响应优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	//6、使能接收中断
	USART_ITConfig(USART3 , USART_IT_RXNE , ENABLE);
	
	//7、启动UART3
	USART_Cmd(USART3 , ENABLE);
}

//中断服务函数--接收数据
void USART3_IRQHandler(void)
{
	u8 data;
	
	if(USART_GetITStatus(USART3 , USART_IT_RXNE))//判断接收中断标志位
	{
		data = USART_ReceiveData(USART3);//接收数据
		
		if(data == '\r' || data == '\n')
			usart3_state = true;
		else
		{
			if(usart3_count < BUF_SIZE - 1)
			{
				usart3_buf[usart3_count++] = data;
			}
			else
			{
				usart3_count  = 0; 
			memset((char *)usart3_buf , 0, BUF_SIZE);
			}
			
		}
		//判断是不是服务器时间
		if(usart3_count == 19 && Is_server_time())
			usart3_state = true;
		USART_ClearITPendingBit(USART3 , USART_IT_RXNE);//清楚中断标志位
	}
}



/*
初始化UART1
RX:PA10
TX:PA9
*/
void UART1_Init(uint32_t BaudRate)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	//1、使能时钟总线
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA , ENABLE);
	
	//2、选择引脚的复用功能为UART1
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource9 , GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA , GPIO_PinSource10 , GPIO_AF_USART1);
	
	//3、配置并初始化引脚
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 |  GPIO_Pin_10;	//配置引脚
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;	//工作模式：复用模式
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;	//频率：高速100MHZ
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;		//输出模式：推挽
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;	//上下拉：浮空
	GPIO_Init(GPIOA , &GPIO_InitStruct);
	
	//4、配置并初始化UART1
	USART_InitStruct.USART_BaudRate = BaudRate;	//波特率
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;	//数据位长度：8bit
	USART_InitStruct.USART_StopBits = USART_StopBits_1;	//停止位
	USART_InitStruct.USART_Parity = USART_Parity_No;	//校验方式：无校验
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;		//工作模式：发送+接收
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//硬件流控：无
	USART_Init(USART1 , &USART_InitStruct);
	
	//5、配置NVIC
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;			//中断向量/通道
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;			//响应优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	//6、使能接收中断
	USART_ITConfig(USART1 , USART_IT_RXNE , ENABLE);
	
	//7、启动UART1
	USART_Cmd(USART1 , ENABLE);
}

//中断服务函数--接收数据
void USART1_IRQHandler(void)
{
	u8 data;
	if(USART_GetITStatus(USART1 , USART_IT_RXNE))//判断接收中断标志位
	{		
		data = USART_ReceiveData(USART1);//接收数据
		
		if(data == '\r' || data == '\n')//消息接结束标志
			usart1_state = true;
		else
		{
			usart1_buf[usart1_count++] = data;
			if(usart1_count >= BUF_SIZE)
			{
				usart1_count  = 0; 
				memset((char *)usart1_buf , 0, BUF_SIZE);
			}
		}
		
		USART_ClearITPendingBit(USART1 , USART_IT_RXNE);//清楚中断标志位
	}
}

//通过串口发送数据
void USART_Send_msg(USART_TypeDef* USARTx , char * msg )
{
	while(msg && *msg)
	{
		USART_SendData(USARTx , *msg++);
		while(USART_GetFlagStatus(USARTx , USART_FLAG_TXE)!= SET);//等待发送结束
	}
}

//printf重定向到USART1
int fputc(int c, FILE * f)
{
	USART_SendData(USART1 , c);
	while(USART_GetFlagStatus(USART1 , USART_FLAG_TXE)!= SET);//等待发送结束
	return c;
}

