#ifndef __BMP280_H
#define __BMP280_H
#include "stm32f4xx.h"



//I2C设备地址，SDO接地=0x76；SDO接3.3V=0x77
#define BME280_ADDR        0x76

#define BME280_REG_CALIB_TP   0x88
#define BME280_REG_CALIB_H1   0xA1
#define BME280_REG_CALIB_H2H6 0xE1

#define BME280_REG_TEMP_MSB   0xFA
#define BME280_REG_PRESS_MSB  0xF7
#define BME280_REG_HUMI_MSB   0xFD

#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_CONFIG     0xF5

#define BH1750_ADDR_WR   0x46U
#define BH1750_ADDR_RD   0x47U

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
}BME280_CalibTypeDef;

void BME280_I2C1_Init(void);
uint8_t BME280_Init(void);
uint8_t BME280_ReadReg(uint8_t reg,uint8_t *buf,uint8_t len);
uint8_t BME280_WriteReg(uint8_t reg,uint8_t val);

int32_t BME280_GetTemp(void);
uint32_t BME280_GetPressure(void);
float BME280_GetHumidity(void);
float BME280_CalcAltitude(uint32_t pressure);

uint16_t BH1750_ReadLux(void);

void BH1750_WriteReg(uint8_t reg_add);
void BH1750_ReadData(unsigned char* Read);

#endif
