#ifndef __RTC_H
#define __RTC_H

#include "stm32f4xx.h"

//时间结构体
typedef struct
{
	uint8_t year;
	uint8_t day;
	uint8_t month;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
}RtcTime_t;

void MyRTC_Init(void);
void MyRTC_SetTime(RtcTime_t *t);
void MyRTC_ReadTime(RtcTime_t *t);

#endif
