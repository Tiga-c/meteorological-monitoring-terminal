#include "BMP280.h"
#include "delay.h"
#include <stdio.h>
#include <math.h>



static BME280_CalibTypeDef calib;
static int32_t t_fine;




//I2C1初始化 PB8=SCL PB9=SDA
void BME280_I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    I2C_InitTypeDef I2C_InitStruct;
    GPIO_InitTypeDef GPIO_Unlock;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // =========【新增：I2C总线解锁，释放SDA锁死】=========
    //先临时配置为推挽输出，手动翻转SCL产生时钟，把从机I2C状态机复位
    GPIO_Unlock.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Unlock.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Unlock.GPIO_OType = GPIO_OType_PP;
    GPIO_Unlock.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_Unlock);

    GPIO_SetBits(GPIOB, GPIO_Pin_7);   //先把SDA拉高
    for(uint8_t i = 0; i < 9; i++)     //输出9个SCL脉冲
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_6);
        delay_us(10);
        GPIO_SetBits(GPIOB, GPIO_Pin_6);
        delay_us(10);
    }
    GPIO_SetBits(GPIOB, GPIO_Pin_7);   //结束前保证SDA高
    // =========解锁结束，下面是原来硬件I2C初始化代码=========

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_I2C1);
    GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_I2C1);

    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = 0x00;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStruct.I2C_ClockSpeed = 100000;
    I2C_Init(I2C1,&I2C_InitStruct);
    I2C_Cmd(I2C1,ENABLE);
}



//I2C1初始化 PB8=SCL PB9=SDA
//void BME280_I2C1_Init(void)
//{
//    GPIO_InitTypeDef GPIO_InitStruct;
//    I2C_InitTypeDef I2C_InitStruct;

//    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
//    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
//    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
//    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
//    GPIO_Init(GPIOB, &GPIO_InitStruct);

//    GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_I2C1);
//    GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_I2C1);

//    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
//    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
//    I2C_InitStruct.I2C_OwnAddress1 = 0x00;
//    I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
//    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
//    I2C_InitStruct.I2C_ClockSpeed = 100000;

//    I2C_Init(I2C1,&I2C_InitStruct);
//    I2C_Cmd(I2C1,ENABLE);
//}

//I2C读多字节
uint8_t BME280_ReadReg(uint8_t reg,uint8_t *buf,uint8_t len)
{
    uint8_t i;
    while(I2C_GetFlagStatus(I2C1,I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1,ENABLE);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1,BME280_ADDR<<1,I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1,reg);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTART(I2C1,ENABLE);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1,BME280_ADDR<<1,I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(i=0;i<len;i++)
    {
        if(i == len-1)
            I2C_AcknowledgeConfig(I2C1,DISABLE);
        else
            I2C_AcknowledgeConfig(I2C1,ENABLE);
        while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_RECEIVED));
        buf[i] = I2C_ReceiveData(I2C1);
    }
    I2C_GenerateSTOP(I2C1,ENABLE);
    I2C_AcknowledgeConfig(I2C1,ENABLE);
    return 0;
}

//I2C写单字节寄存器
uint8_t BME280_WriteReg(uint8_t reg,uint8_t val)
{
    while(I2C_GetFlagStatus(I2C1,I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1,ENABLE);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1,BME280_ADDR<<1,I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    I2C_SendData(I2C1,reg);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_SendData(I2C1,val);
    while(!I2C_CheckEvent(I2C1,I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1,ENABLE);
    return 0;
}

//BME280初始化，读取校准参数
//BME280初始化，读取校准参数【修复版，增加0xF2湿度寄存器配置】
uint8_t BME280_Init(void)
{
    uint8_t buf[24];
    BME280_I2C1_Init();

    //软件复位
    BME280_WriteReg(0xE0,0xB6);
    delay_ms(20);

    BME280_ReadReg(BME280_REG_CALIB_TP,buf,24);
    calib.dig_T1 = (buf[1]<<8)|buf[0];
    calib.dig_T2 = (buf[3]<<8)|buf[2];
    calib.dig_T3 = (buf[5]<<8)|buf[4];

    calib.dig_P1 = (buf[7]<<8)|buf[6];
    calib.dig_P2 = (buf[9]<<8)|buf[8];
    calib.dig_P3 = (buf[11]<<8)|buf[10];
    calib.dig_P4 = (buf[13]<<8)|buf[12];
    calib.dig_P5 = (buf[15]<<8)|buf[14];
    calib.dig_P6 = (buf[17]<<8)|buf[16];
    calib.dig_P7 = (buf[19]<<8)|buf[18];
    calib.dig_P8 = (buf[21]<<8)|buf[20];
    calib.dig_P9 = (buf[23]<<8)|buf[22];

    //！！BMP280没有湿度寄存器0xF2！删掉那一行！
    BME280_WriteReg(BME280_REG_CTRL_MEAS,0x27);
    BME280_WriteReg(BME280_REG_CONFIG,0x04);

    delay_ms(50);
    return 0;
}


//读取温度 返回值 ×100（例如2510代表25.10℃）
int32_t BME280_GetTemp(void)
{
    uint8_t buf[3];
    int32_t adc_T;
    int32_t temp;

    BME280_ReadReg(BME280_REG_TEMP_MSB,buf,3);
    adc_T = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | ((int32_t)buf[2] >> 4);

    //----------正确官方BMP280温度补偿算法----------
    int32_t var1,var2;
    var1 = ((((adc_T>>3) - ((int32_t)calib.dig_T1 <<1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((int32_t)calib.dig_T1)) * ((adc_T>>4) - ((int32_t)calib.dig_T1))) >>12 ) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;

    temp = (t_fine * 5 + 128) >> 8;
    return temp;
}



//读取气压，返回Pa
uint32_t BME280_GetPressure(void)
{
    uint8_t buf[3];
    int32_t adc_P;
    int64_t var1,var2,p;
    BME280_GetTemp();
    BME280_ReadReg(BME280_REG_PRESS_MSB,buf,3);
    adc_P = ((int32_t)buf[0]<<12)|((int32_t)buf[1]<<4)|((int32_t)buf[2]>>4);

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1*(int64_t)calib.dig_P5)<<17);
    var2 = var2 + (((int64_t)calib.dig_P4)<<35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3)>>8) + ((var1 * (int64_t)calib.dig_P2)<<12);
    var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calib.dig_P1)>>33;

    if(var1 == 0) return 0;
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((int64_t)calib.dig_P9) * (p>>13) * (p>>13)) >>25;
    var2 = (((int64_t)calib.dig_P8)*p)>>19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7)<<4);
    return (uint32_t)(p>>8);
}


//计算海拔高度(米)，标准海平面气压101325Pa
float BME280_CalcAltitude(uint32_t pressure)
{
    const float P0 = 101325.0f;
    float alt;
    alt = 44330.0f * ( 1.0f - powf( (float)pressure / P0 , 1.0f/5.255f ) );
    return alt;
}


//光照强度
//写入命令到BH1750
#define I2C1_SLAVE_ADDRESS7      0x23U  //ADDR引脚接地
void BH1750_WriteReg(uint8_t reg_add)
{
    /* wait until I2C bus is idle */
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    /* send a start condition to I2C bus */
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    /* send slave address to I2C bus, transmit */
    I2C_Send7bitAddress(I2C1, I2C1_SLAVE_ADDRESS7 << 1, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    /* send a data byte */
    I2C_SendData(I2C1, reg_add);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    I2C_GenerateSTOP(I2C1, ENABLE);
}

//读取BH1750光照数据(2字节)
void BH1750_ReadData(unsigned char* Read)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    /* send slave address, receiver */
    I2C_Send7bitAddress(I2C1, I2C1_SLAVE_ADDRESS7 << 1, I2C_Direction_Receiver);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    Read[0] = I2C_ReceiveData(I2C1);
	
		I2C_AcknowledgeConfig(I2C1, DISABLE);  //关闭应答，准备接收最后字节
    __NOP();

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    Read[1] = I2C_ReceiveData(I2C1);

    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}
