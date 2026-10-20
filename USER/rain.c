#include "stm32f4xx.h"
#include "rain.h"
#include <stdint.h>
#include "delay.h"

void RAIN_PB0_ADC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    //只开GPIOB时钟，不要操作ADC1！
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}


uint16_t ADC_Read_Channel(uint8_t ch)
{
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC); //读完手动清除标志！！
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_56Cycles);
    ADC_SoftwareStartConv(ADC1);

    uint32_t timeout = 10000;
    while((ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) && (timeout--));
    if(timeout == 0)
    {
        return 0;
    }
		uint16_t adc_val = ADC_GetConversionValue(ADC1); //第一步：读取采样数值
		ADC_ClearFlag(ADC1, ADC_FLAG_EOC);               //第二步：读完之后清除标志
    return adc_val;
}




uint16_t adc_dry = 0;   // 雨滴传感器上电校准：干燥状态下的ADC数字基准值

/**
 * @brief  Rain_Sensor_Calibrate
 *  自适应校准函数，【上电只调用1次】！！前提：上电时雨滴检测板必须是干燥无水
 *  功能：多次采集ADC，取平均值，作为干燥基准，抵消模块个体差异、灰尘漂移
 *  调用时机：ADC初始化完成、启动转换之后，while(1)之前执行一次
 */
void Rain_Sensor_Calibrate(void)
{
    uint32_t sum = 0;               // 8次采样总和，用32位防止累加溢出
    uint16_t buf[8];                // 存放8次ADC采样的数字结果，不是电压！

    // 连续采集8次ADC，做简单多次采样滤波，降低噪声抖动
    for(uint8_t i = 0; i < 8; i++)
    {
        // HAL_ADC_GetValue()：读取ADC转换完成后的数字量，范围0~4095，不是电压
        buf[i] = ADC_Read_Channel(ADC_Channel_8);

        delay_ms(10);   // 调用你自己写的毫秒阻塞延时，间隔10ms再采下一次
    }

    // 将8次采样数值累加
    for(uint8_t i = 0; i < 8; i++)
    {
        sum += buf[i];
    }

    // 求平均值，得到干燥状态下的基准ADC数字量
    adc_dry = sum / 8U;

    /*
        异常兜底保护：
        如果上电不巧板子是潮湿的，采样得到的adc_dry会偏小，限制合理区间
        正常干燥YL‑83模块：adc_dry应该在3200 ~ 4095之间
        若低于下限，强制赋值一个默认干燥基准，防止逻辑完全错乱
    */
    if(adc_dry < 3200)
        adc_dry = 3800;
    if(adc_dry > 4095)
        adc_dry = 4095;
}

/**
 * @brief  Rain_Get_Level
 * @retval uint8_t  0=干燥  1=湿润  2=小雨  3=中雨  4=大雨
 * 函数逻辑：不使用固定ADC阈值，使用【相对偏移量delta】判断雨量
 * adc_dry：干燥基准数字值；adc_val：当前实时ADC数字值
 * delta = adc_dry - adc_val
 *      板子越湿 → adc_val越小 → delta差值越大
 */
float Rain_Get_WetPercent(void)
{
    uint32_t sum = 0;
    //连续采8次求平均，抑制ADC噪声
    for(uint8_t i=0;i<8;i++)
    {
        sum += ADC_Read_Channel(ADC_Channel_5);
        delay_ms(2);
    }
    uint16_t adc_val = sum / 8U;

    int32_t delta = (int32_t)adc_dry - (int32_t)adc_val;

    if(delta < 0)
        delta = 0;

    if(adc_dry == 0)
    {
        return 0.0f;
    }

    float wet_percent = (delta * 100.0f) / adc_dry;

    if(wet_percent > 100.0f)
        wet_percent = 100.0f;

    return wet_percent;
}

