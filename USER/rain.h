#ifndef __RAIN_H__
#define __RAIN_H__
#include "stm32f4xx.h" 
uint16_t ADC_Read_Channel(uint8_t ch);
void RAIN_PB0_ADC_Init(void);
float Rain_Get_WetPercent(void);
void Rain_Sensor_Calibrate(void);
extern uint16_t adc_dry;
#endif
