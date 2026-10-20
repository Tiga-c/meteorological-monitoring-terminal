#ifndef _ESP_01S_H_
#define _ESP_01S_H_

#include "stm32f4xx.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>
#include "delay.h"

#define BUF_SIZE 256

#include "secrets.h"

/*
TCP连接命令
*/
#define SUB 1	//订阅
#define PUB 2	//发布
#define SUB_HIS 3	//订阅+一条历史记录
#define GET_TIME 7	//获取服务器时间
#define GET_HIS 9	//获取一条最新历史记录
extern uint8_t wifi_is_ok;

//wifi模块初始化状态机
typedef enum {
			AT ,	//AT测试
			CWMODE ,//设置工作模式
			CWJAP ,	//连接热点
			CIFSR ,	//打印自己的IP
			CIPSTART, //链接TCP server
			CIPMODE,//开启透传模式
			CIPSEND//进入透传
			}State;

/*
初始化UART1
RX:PA10
TX:PA9
*/
void UART1_Init(uint32_t BaudRate);

/*
初始化UART3
RX:PB11
TX:PB10
*/
void UART3_Init(uint32_t BaudRate);

//通过串口发送数据
void USART_Send_msg(USART_TypeDef* USARTx , char * msg );
			
			
/*
ESP-01S初始化：
设置为station模式，连接热点，进入透传，连接TCP服务器
*/
void ESP_01S_Link_AP_and_Server(void);

/*
发送数据
*/
void ESP_O1S_send_msg(char * msg);

/*
发送命令
cmd :消息类型
	1	订阅消息：订阅主题，接收消息
	2	发布消息：向主题发送消息
	3	订阅+历史：订阅主题并拉取最新消息
	7	获取时间：获取服务器时间
	9	获取历史：拉取主题最新消息
topic：主题名称（设备标识）
msg：消息内容
*/
void ESP_send_command(u8 cmd , char * topic  , char * msg);

//心跳包
void heartbeat(void);

#endif
