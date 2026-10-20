#ifndef __FLASH_H
#define __FLASH_H
#include "stm32f4xx.h"
#include "RTC.h"
typedef struct
{
	uint8_t magic;      //====【新增！日志有效标记 0xAA代表有效日志】====
	float PCB_wet;
	float temp;
	uint8_t Humidity;
	float uv_power;
	uint32_t LightData_Hex;
	float alt;
	float press;
	RtcTime_t collect_time;
}SensorLogTypeDef;
uint16_t Get_Log_Count(void);
void Flash_Save_SensorLog(SensorLogTypeDef *pLog);
void Flash_Read_SensorLog(uint16_t idx, SensorLogTypeDef *pLog);
void Flash_Erase_LogSector(void);
#endif
