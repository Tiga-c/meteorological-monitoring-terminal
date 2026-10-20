#include "stm32f4xx.h"
#include <stdbool.h>
#include "led.h"
#include "delay.h"
#include "DHT11.h"
#include "sys.h"
#include "S12SD.h"
#include "rain.h"
#include "oled.h"
#include "bmp.h"
#include "ESP_01S.h"
#include "TIMER.h"
#include "BMP280.h"
#include "scheduler.h"
#include "LoRa.h"
#include "flash.h"
#include "RTC.h"

extern  char usart1_buf[BUF_SIZE] , usart3_buf[BUF_SIZE];	//串口1接收并保存数据的缓冲区
extern volatile bool usart1_state , usart3_state;			//串口1的接收结束标志
extern volatile u8 usart1_count, usart3_count;			//串口1接收数据量
extern volatile u8 state;
extern u32 esp_data_len;       // 解析后的数据长度
extern u8 *esp_data_ptr;    // 解析后的数据指针
extern u16 HeartBeat;
void oled_show(float PCB_wet, int32_t temp, u8 Humidity, float uv_power,float alt,float press,float LightData_Hex);

u32 upload_timer = 0; 		//巴法云上报计时器
	const u32 upload_period = 2000;	//2000ms上报一次
	char publish_buf[32]; 		//上报字符串缓冲区，加大容量存多个传感器数据
	char publish_buf1[32];
	char publish_buf2[32];
	char publish_buf3[32];
	char publish_buf4[32];
	char publish_buf5[32];
	char publish_buf6[32];
	char msg[5];
	





extern uint8_t lora_is_ok;
extern uint8_t wifi_is_ok;
/* ======================= 巴法云上报 ======================= */
void Bafa_Send(float PCB_wet, float temp, u8 Humidity, float uv_power,
               uint32_t LightData_Hex, float alt, float press)
{
	if(wifi_is_ok)
	{
		snprintf(publish_buf, sizeof(publish_buf), "%.2f", (temp/100)-4);
    printf("【巴法云上报温度】%s\r\n", publish_buf);
    ESP_send_command(PUB, TOPIC, publish_buf);
    delay_ms(100);

    snprintf(publish_buf1, sizeof(publish_buf1), "%d", Humidity);
    printf("【巴法云上报湿度】%s\r\n", publish_buf1);
    ESP_send_command(PUB, TOPIC1, publish_buf1);
    delay_ms(100);

    snprintf(publish_buf2, sizeof(publish_buf2), "%.2f", uv_power);
    printf("【巴法云上报紫外线强度】%s\r\n", publish_buf2);
    ESP_send_command(PUB, TOPIC2, publish_buf2);
    delay_ms(100);

    snprintf(publish_buf3, sizeof(publish_buf3), "%.2f", PCB_wet);
    printf("【巴法云上报湿润度】%s\r\n", publish_buf3);
    ESP_send_command(PUB, TOPIC3, publish_buf3);
    delay_ms(100);

    snprintf(publish_buf4, sizeof(publish_buf4), "%d", (int)LightData_Hex);
    printf("【巴法云上报光照强度】%s\r\n", publish_buf4);
    ESP_send_command(PUB, TOPIC5, publish_buf4);
    delay_ms(100);

    snprintf(publish_buf5, sizeof(publish_buf5), "%.2f", alt);
    printf("【巴法云上报海拔】%s\r\n", publish_buf5);
    ESP_send_command(PUB, TOPIC6, publish_buf5);
    delay_ms(100);

    snprintf(publish_buf6, sizeof(publish_buf6), "%.2f", press/100);
    printf("【巴法云上报大气压强】%s\r\n", publish_buf6);
    ESP_send_command(PUB, TOPIC7, publish_buf6);
    delay_ms(100);
	}
		
	else
		return;
   
}


/* ======================= 串口打印 ======================= */
void Printf_show(float PCB_wet, float temp, u8 Humidity, float uv_power,
                 uint32_t LightData_Hex, float alt, float press,
                 uint16_t adc_raw, float voltage)
{
    if (PCB_wet < 1.0f)         printf("①极板干燥\n");
    else if (PCB_wet < 15.0f)   printf("①轻微沾水\n");
    else                        printf("①明显沾水\n");

    printf("紫外线强度:%.2f mW/cm²\r\n", uv_power);

    if      (uv_power >= 0  && uv_power <= 5)   printf("紫外线弱\r\n");
    else if (uv_power > 5   && uv_power <= 20)  printf("紫外线中等\r\n");
    else if (uv_power > 20  && uv_power <= 50)  printf("紫外线较强\r\n");
    else if (uv_power > 50)                     printf("紫外线极强\r\n");

    printf("光照强度：%lu lux\r\n", (unsigned long)LightData_Hex);
    printf("温度：%.2f C\r\n", temp/100);
    printf("海拔：%.2f m\r\n", alt);
    printf("大气压：%.2f Pa\r\n", press/100);
}


/* ======================= 采集任务（周期 12s）=======================
 * ================================================================ */
void sensor_collect_task(void)
{
	//==== 网络恢复之后优先补发离线日志 ====
    if(wifi_is_ok == 1)
    {
        uint16_t log_cnt = Get_Log_Count();
        if(log_cnt > 0)
        {
            printf("[Task]检测到离线缓存，开始补发历史数据！total=%d\r\n",log_cnt);
            SensorLogTypeDef temp_log;
            for(uint16_t i=0;i<log_cnt;i++)
            {
                Flash_Read_SensorLog(i,&temp_log);
                Bafa_Send(temp_log.PCB_wet, temp_log.temp, temp_log.Humidity,
                          temp_log.uv_power, temp_log.LightData_Hex, temp_log.alt, temp_log.press);
                delay_ms(600); //上报之间延时，防止WIFI模块发太快卡死
            }
            Flash_Erase_LogSector(); //全部补发完成之后擦除缓存扇区
            printf("[Task]离线日志补发完毕，flash缓存已清空\r\n");
        }
    }
	
    uint8_t  DataBuff[2];
    float    LightData;
    uint32_t LightData_Hex;
    float    temp, press, alt, PCB_wet;
    u8       Humidity = 67;      /* DHT11 没接的话先给固定值 */
    uint16_t adc_raw;   
    float    voltage, uv_power;
	
		  
		

    printf("\r\n===== [Task] start =====\r\n");

    /* BME280 */
    temp  = BME280_GetTemp();
    press = BME280_GetPressure();
    alt   = BME280_CalcAltitude(press);
    printf("[Task] BME280: T=%.2f P=%.2f alt=%.2f\r\n", temp/100, press, alt);

    /* BH1750 */
    BH1750_WriteReg(0x01);
    BH1750_WriteReg(0x10);
    delay_ms(10);
    BH1750_ReadData(DataBuff);
    LightData     = ((DataBuff[0] << 8) + DataBuff[1]) / 1.2f;
    LightData_Hex = (uint32_t)LightData;
    printf("[Task] BH1750: lux=%lu\r\n", (unsigned long)LightData_Hex);

    /* 雨滴 */
    PCB_wet = Rain_Get_WetPercent();
    printf("[Task] Rain: wet=%.2f\r\n", PCB_wet);

    /* 紫外线 ADC */
    adc_raw  = ADC_Read_Channel(ADC_Channel_4);
    voltage  = adc_raw * 3.3f / 4095.0f;
    uv_power = voltage * 250.0f;
    if (uv_power < 0) uv_power = 0;
    printf("[Task] UV: adc=%u v=%.2f uv=%.2f\r\n", adc_raw, voltage, uv_power);

    /* 打印 */
    printf("[Task] -> Printf_show\r\n");
    Printf_show(PCB_wet, temp, Humidity, uv_power, LightData_Hex, alt, press, adc_raw, voltage);
		
		//=======链路选择逻辑（wifi_is_ok全局标志位）=======
    if(wifi_is_ok == 1)
    {
        //WIFI正常：走巴法云服务器上报，不使用LoRa
        printf("[Task] WIFI_OK -> 巴法云上报\r\n");
        Bafa_Send(PCB_wet, temp, Humidity, uv_power, LightData_Hex, alt, press);
    }
    else
    {
					if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0) == 0) 
					{
						lora_is_ok = 1;
						printf("[Task] WIFI_LOST -> 使用LoRa备份上报\r\n");
						LoRa_Send(PCB_wet, temp,Humidity, uv_power,LightData_Hex, alt, press);
					}
					else 
					{
						lora_is_ok = 0;
					}
					
					
					printf("lora:%d\n",lora_is_ok);
					printf("电平为：%d\n",GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0));
				
				if(lora_is_ok == 0)
				{
					//走flash					
					SensorLogTypeDef log_item;
					//把本次采集到的各个传感器变量赋值进去
					log_item.magic = 0xAA;
					log_item.PCB_wet = PCB_wet;
					log_item.temp = temp;
					log_item.Humidity = Humidity;
					log_item.uv_power = uv_power;
					log_item.LightData_Hex = LightData_Hex;
					log_item.alt = alt;
					log_item.press = press;
					//读取RTC采集时间
					MyRTC_ReadTime(&log_item.collect_time);
					printf("Time:20%02d-%02d-%02d %02d:%02d-%02d\r\n",
						log_item.collect_time.year,
						log_item.collect_time.month,
						log_item.collect_time.day,
						log_item.collect_time.hour,
						log_item.collect_time.min,
						log_item.collect_time.sec);
					printf("[Task] WIFI & LoRa both down -> save data into flash!\r\n");
					Flash_Save_SensorLog(&log_item);
				
    }

    /* OLED 显示 */
    printf("[Task] -> oled_show\r\n");
    
		oled_show(PCB_wet, (int32_t)(temp/100.0f), Humidity, uv_power,alt,(int32_t)(press/100),LightData_Hex);
    printf("===== [Task] end =====\r\n\r\n");
}
		}


/* ======================= main ======================= */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init();
    LED_init();

    /* --- 硬件初始化：上电只做一次 --- */
    OLED_Init();
    OLED_Clear();
		MyRTC_Init();
		LoRa_GPIO_Init();
		delay_ms(100);
    ADC1_GPIO_Config();
    ADC1_Init();
    RAIN_PB0_ADC_Init();

    UART1_Init(115200);  //串口   
    UART3_Init(115200);  //WIFI 
		UART2_Init(9600);    //LoRa
	
//		Flash_Erase_LogSector();
//		printf("擦除测试");


    delay_ms(200);
    Rain_Sensor_Calibrate();

    OLED_Clear();
    OLED_DrawBMP(0, 0, 128, 8, &BmpTest3[0]);
    delay_ms(800);
    OLED_Clear();
		float PCB_wet;
		int32_t Temperature;
		u8 Humidity;
		float uv_power;

    BME280_Init();
    BME280_I2C1_Init();

    BH1750_WriteReg(0x01);
    BH1750_WriteReg(0x10);
    delay_ms(180);

    delay_ms(500);

    /* --- 连 WIFI / TCP，只跑一次 --- */
    ESP_01S_Link_AP_and_Server();
    delay_ms(200);

    printf("\r\n[main] ESP link done, starting scheduler...\r\n");

    /* --- 调度器初始化 --- */
    printf("[main] event_init...\r\n");
    event_init();

    printf("[main] scheduler_init (含 TIM6 tick 启动)...\r\n");
    scheduler_init();

    printf("[main] add task (period=12000ms)...\r\n");
    scheduler_add_task(sensor_collect_task, 12000);

    printf("[main] scheduler_run...\r\n");
    scheduler_run();          /* 永不返回 */

    while (1) { 
		
		}
}


/* ======================= OLED 显示 ======================= */
void oled_show(float PCB_wet, int32_t temp, u8 Humidity, float uv_power,float alt,float press,float LightData_Hex)
{
    /* 第 1 屏：湿润度 */
    OLED_ShowString(0, 0, "Weather Monitor", 16);
    OLED_ShowCHinese(0,  2, 19);
    OLED_ShowCHinese(16, 2, 21);
    OLED_ShowCHinese(32, 2, 20);
    OLED_ShowString(48, 2, ":", 16);
    OLED_ShowNum(64, 2, (u16)PCB_wet, 2, 16);
    OLED_ShowString(80, 2, "%", 16);
    delay_ms(1000);
    OLED_Clear();

    /* 第 2 屏：温度湿度 */
    OLED_ShowString(0, 0, "Weather Monitor", 16);
    OLED_ShowCHinese(0,  2, 18);
    OLED_ShowString(16, 2, ":", 16);
    OLED_ShowNum(24, 2, temp, 2, 16);
    OLED_ShowString(40, 2, "C ", 16);
    OLED_ShowCHinese(56, 2, 19);
    OLED_ShowString(72, 2, ":", 16);
		OLED_ShowString(80, 2, "67", 16);
    
    OLED_ShowString(96, 2, "%", 16);
    delay_ms(1000);
    OLED_Clear();

    /* 第 3 屏：紫外线等级 */
    OLED_ShowString(0, 0, "Weather Monitor", 16);
    OLED_ShowCHinese(0,  2, 0);
    OLED_ShowCHinese(16, 2, 1);
    OLED_ShowCHinese(32, 2, 2);
    OLED_ShowString(48, 2, ":", 16);

    if(uv_power >= 0  && uv_power <= 5)  
		{ 
			OLED_ShowCHinese(80, 2, 3); 
		}
		else if (uv_power > 5   && uv_power <= 20)
		{ 
			OLED_ShowCHinese(80, 2, 4); 
		}
		else if (uv_power > 20  && uv_power <= 50) 
		{ 
			OLED_ShowCHinese(80, 2, 6); OLED_ShowCHinese(96, 2, 7); 
		}
		else if (uv_power > 50)
		{ 
			OLED_ShowCHinese(80, 2, 8); 
			OLED_ShowCHinese(96, 2, 7); 
		}
		delay_ms(1000);
	
		OLED_ShowString(0, 0, "Weather Monitor", 16);
		OLED_ShowString(0, 2, "light intensity", 16);

		OLED_ShowNum(0, 4, LightData_Hex, 4, 16);
		delay_ms(1000);
		OLED_Clear();
		
		OLED_ShowString(0, 0, "Weather Monitor", 16);
		OLED_ShowString(0, 2, "altitude", 16);
		OLED_ShowNum(0, 4, alt, 4, 16);
		delay_ms(1000);
		OLED_Clear();
		
		OLED_ShowString(0, 0, "Weather Monitor", 16);
		OLED_ShowString(0, 2, "air pressure", 16);
		OLED_ShowNum(0, 4, press, 4, 16);
		delay_ms(1000);
		
		
    
    OLED_Clear();
    delay_ms(2000);
}

//void Bafa_Send(float PCB_wet,float temp,u8 Humidity,float uv_power,uint32_t LightData_Hex,float alt,float press)
//{
//			snprintf(publish_buf, sizeof(publish_buf),
//				"%.2f",
//				temp/100);
//			printf("【巴法云上报温度】%s\r\n", publish_buf);
//			ESP_send_command(PUB , TOPIC , publish_buf);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf1, sizeof(publish_buf1),
//				"%d",
//				67);
//			printf("【巴法云上报湿度】%s\r\n", publish_buf1);
//			ESP_send_command(PUB , TOPIC1 , publish_buf1);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf2, sizeof(publish_buf2),
//				"%.2f",
//				uv_power);
//			printf("【巴法云上报紫外线强度】%s\r\n", publish_buf2);
//			ESP_send_command(PUB , TOPIC2 , publish_buf2);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf3, sizeof(publish_buf3),
//				"%.2f",
//				PCB_wet);
//			printf("【巴法云上报湿润度】%s\r\n", publish_buf3);
//			ESP_send_command(PUB , TOPIC3 , publish_buf3);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf4, sizeof(publish_buf4),
//				"%d",
//				LightData_Hex);
//			printf("【巴法云上报光照强度】%s\r\n", publish_buf4);
//			ESP_send_command(PUB , TOPIC5 , publish_buf4);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf5, sizeof(publish_buf5),
//				"%.2f",
//				alt);
//			printf("【巴法云上报海拔】%s\r\n", publish_buf5);
//			ESP_send_command(PUB , TOPIC6 , publish_buf5);
//			delay_us(100);
////			delay_ms(200);
//			
//			snprintf(publish_buf6, sizeof(publish_buf6),
//				"%.2f",
//				press/100);
//			printf("【巴法云上报大气压强】%s\r\n", publish_buf6);
//			ESP_send_command(PUB , TOPIC7 , publish_buf6);
//			delay_us(100);
////			delay_ms(200);
//			
//		
//}

//void Printf_show(float PCB_wet,float temp,u8 Humidity,float uv_power,uint32_t LightData_Hex,float alt,float press,uint16_t adc_raw,float voltage)
//{
//		//=========== 串口打印 ===========
//		if(PCB_wet < 1.0f)
//		{
//				printf("①极板干燥\n");
//		}
//		else if(PCB_wet <15.0f)
//		{
//				printf("①轻微沾水\n");
//		}
//		else
//		{
//				printf("①明显沾水\n");
//		}
//		
//		printf("紫外线强度:%.2f mW/cm²  \r\n",uv_power);
//	
//			
//		if(uv_power>=0 && uv_power<=5)
//			printf("紫外线弱\r\n");
//		else if(uv_power>5 && uv_power<=20)
//			printf("紫外线中等\r\n");
//		else if(uv_power>20 && uv_power<=50)
//			printf("紫外线较强\r\n");
//		else if(uv_power>50)
//			printf("紫外线极强\r\n");
//			
//		
//		
//		printf("光照强度：%d lux\r\n",LightData_Hex);
//		printf("温度：%.2f C\r\n",temp/100);
//		printf("海拔：%.2f m\r\n",alt);
//		printf("大气压：%.2f Pa\r\n",press/100);
//}




//void data_acquisition(float PCB_wet,float temp,u8 Humidity,float uv_power,uint32_t LightData_Hex,float alt,float press)
//{
//	uint8_t DataBuff[2];
//	temp  = BME280_GetTemp();       //×100
//  press = BME280_GetPressure();   //Pa
//  alt   = BME280_CalcAltitude(press);
//	
//		BH1750_WriteReg(0x01);    //power on
//		BH1750_WriteReg(0x10);    //H‑resolution mode
//			
////		delay_ms(180);

//    BH1750_ReadData(DataBuff);
//    float LightData = ((DataBuff[0] <<8) + DataBuff[1]) / 1.2f;
//    LightData_Hex = (uint32_t)LightData;

//		PCB_wet = Rain_Get_WetPercent();
//}



//void sensor_collect_task(void)
//{
//	uint8_t dummy;
//	
//	uint8_t DataBuff[2];
//	float LightData;
//	uint32_t LightData_Hex;
//	float temp;
//	float press;
//	float alt;
//	float PCB_wet;
//	u8 Humidity;
//	uint16_t adc_raw;
//	float voltage, uv_power;
//	
////	UART1_Init(115200);
////	UART3_Init(115200);
//	printf("hello!!!!!");

//    //唤醒之后重新初始化I2C，防止休眠后I2C挂死
//	BME280_I2C1_Init();
//	
//	while(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == SET)
//	{
//		dummy = USART_ReceiveData(USART1);
//	}
//	//清除ORE过载错误标志（休眠唤醒极易出现ORE错误！！！）
//	if(USART_GetFlagStatus(USART1,USART_FLAG_ORE) == SET)
//	{
//		USART_ClearFlag(USART1,USART_FLAG_ORE);
//	}

//	temp  = BME280_GetTemp();       //温度 ×100
//	press = BME280_GetPressure();   //压强 Pa
//	alt   = BME280_CalcAltitude(press);

//	BH1750_WriteReg(0x01);    //power on
//	BH1750_WriteReg(0x10);    //H‑resolution mode
////	delay_ms(180);
//	BH1750_ReadData(DataBuff);
//	LightData = ((DataBuff[0] <<8) + DataBuff[1]) / 1.2f;
//	LightData_Hex = (uint32_t)LightData;

//	PCB_wet = Rain_Get_WetPercent();

//	//读取紫外线ADC
//	adc_raw = ADC_Read_Channel(ADC_Channel_4);
//	voltage = adc_raw * 3.3f / 4095.0f;
//	uv_power = voltage * 250.0f;
//	if(uv_power < 0) uv_power = 0;


//	Printf_show(PCB_wet,temp,Humidity,uv_power,LightData_Hex,alt,press,adc_raw,voltage);
//	Bafa_Send(PCB_wet,temp,Humidity,uv_power,LightData_Hex,alt,press);
//	oled_show(PCB_wet,(int32_t)(temp/100.0f),Humidity,uv_power);
//}



//int main(void)
//{
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	delay_init();
//	LED_init();
//	// 硬件初始化顺序：延时、中断分组放最前面！
//	OLED_Init();			//初始化OLED
//	OLED_Clear(); 		//只在这里上电清一次屏
//	ADC1_GPIO_Config();
//	ADC1_Init();        //ADC1只初始化这一次
//	RAIN_PB0_ADC_Init();//仅配置PB0 GPIO，不碰ADC外设
//	UART1_Init(115200);
//	UART3_Init(115200);
//	
//	
//	delay_ms(200);
//	Rain_Sensor_Calibrate();
//	
//	OLED_Clear();
//	OLED_DrawBMP(0,0, 128, 8, &BmpTest3[0]);
//	delay_ms(800);
//	OLED_Clear();
//	
//	uint8_t DataBuff[2];
//	float LightData;
//	uint32_t LightData_Hex;

//  BME280_Init();
//	BME280_I2C1_Init();

//  delay_ms(200);
//	
//	BH1750_WriteReg(0x01);    //power on
//  BH1750_WriteReg(0x10);    //H‑resolution mode
//  delay_ms(180);
//	
//	BME280_I2C1_Init ();
//	float temp;
//	float press;
//	float humi;
//	float alt;
//	
//	BME280_Init ();
//	

//  delay_ms(500);
//	
//	//【变量全部放main开头】
//	float PCB_wet;
//	u8 Humidity, Temperature;
//	uint16_t adc_raw;
//	float voltage, uv_power;
//	
//	
//	

//	// 启动ESP连接巴法云，放在while外面，只执行一次
//	ESP_01S_Link_AP_and_Server();
//	delay_ms(200); 
//	

//	while(1) // 所有采集代码放到while循环内部！！
//	{ 
//		
//		temp  = BME280_GetTemp();
//    press = BME280_GetPressure();
//    alt   = BME280_CalcAltitude(press);
//			
//		BH1750_WriteReg(0x01);    //power on
//		BH1750_WriteReg(0x10);    //H‑resolution mode
//			
//		delay_ms(180);

//    BH1750_ReadData(DataBuff);
//    LightData = ((DataBuff[0] <<8) + DataBuff[1]) / 1.2f;
//    LightData_Hex = (uint32_t)LightData;

//		PCB_wet = Rain_Get_WetPercent();
//		
//		//采集数据
//		data_acquisition(PCB_wet,temp,Humidity,uv_power,LightData_Hex,alt,press);
//		
//	//====================串口打印===================
//		Printf_show(PCB_wet,temp,Humidity,uv_power,LightData_Hex,alt,press,adc_raw,voltage);
//		

//		//====================巴法云上报计时逻辑====================
//		
//		Bafa_Send(PCB_wet,temp,Humidity,uv_power,LightData_Hex,alt,press);
//		
//		//=========================================================
//		
//		//oled显示模块
//		oled_show(PCB_wet,temp,Humidity,uv_power);
//	}
//}


//void  oled_show(float PCB_wet,int32_t Temperature,u8 Humidity,float uv_power)
//{
//	//极板湿润度 OLED页面
//		OLED_ShowString(0,0,"Weather Monitor",16);
//		OLED_ShowCHinese(0,2,19);  //湿
//		OLED_ShowCHinese(16,2,21); //润
//		OLED_ShowCHinese(32,2,20); //度
//		OLED_ShowString(48,2,":",16);
//		OLED_ShowNum(64,2,(u16)PCB_wet,2,16);
//		OLED_ShowString(80,2,"%",16);
//		delay_ms(1000);
//		OLED_Clear();
//		
//		//温度湿度 OLED页面
//		OLED_ShowString(0,0,"Weather Monitor",16);
//		OLED_ShowCHinese(0,2,18);  //温
//		OLED_ShowString(16,2,":",16);
//		OLED_ShowNum(24,2,Temperature,2,16);
//		OLED_ShowString(40,2,"C ",16);
//		
//		OLED_ShowCHinese(56,2,19); //湿
//		OLED_ShowString(72,2,":",16);
//		OLED_ShowNum(80,2,Humidity,2,16);
//		OLED_ShowString(96,2,"%",16);
//		delay_ms(1000);
//		OLED_Clear();
//		
//		//紫外线 OLED页面
//		OLED_ShowString(0,0,"Weather Monitor",16);
//		OLED_ShowCHinese(0,2,0);  //紫
//		OLED_ShowCHinese(16,2,1); //外
//		OLED_ShowCHinese(32,2,2); //线
//		OLED_ShowString(48,2,":",16);

//		//紫外线等级汉字
//		if(uv_power>=0 && uv_power<=5)
//		{
//			OLED_ShowCHinese(80,2,3); //弱
//		}
//		else if(uv_power>5 && uv_power<=20)
//		{
//			OLED_ShowCHinese(80,2,4); //中
//		}
//		else if(uv_power>20 && uv_power<=50)
//		{
//			OLED_ShowCHinese(80,2,6); //较
//			OLED_ShowCHinese(96,2,7); //强
//		}
//		else if(uv_power>50)
//		{
//			OLED_ShowCHinese(80,2,8); //极
//			OLED_ShowCHinese(96,2,7); //强
//		}
//		delay_ms(1000);
//		OLED_Clear();
//		
//		delay_ms(2000);
//	
//}


