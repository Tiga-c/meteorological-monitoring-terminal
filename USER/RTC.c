#include "RTC.h"

/**
 * @brief  RTC初始化
 * 注意：RTC使用LSE外部低速晶振32.768KHz，原理图Y1就是32.768K
 * 如果后备电池有电，不需要重新设置时间；
 * 如果电池没电，第一次上电需要调用MyRTC_SetTime设置初始时间
 */
void MyRTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);	//使能PWR时钟
	PWR_BackupAccessCmd(ENABLE);		//解除备份域写保护，才可以修改RTC寄存器

	//判断LSE是否已经开启
	if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
	{
		RCC_LSEConfig(RCC_LSE_ON);		//开启外部低速晶振LSE(32768Hz)
		while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); //等待LSE稳定
	}

	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);	//选择LSE作为RTC时钟源
	RCC_RTCCLKCmd(ENABLE);						//打开RTC时钟


}

/**
 * @brief 设置RTC时间
 * @param t:传入时间结构体，year是后两位(26代表2026)
 */
void MyRTC_SetTime(RtcTime_t *t)
{
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;

	RTC_DateStruct.RTC_Year    = t->year;
	RTC_DateStruct.RTC_Month   = t->month;
	RTC_DateStruct.RTC_Date    = t->day;
	RTC_DateStruct.RTC_WeekDay = RTC_Weekday_Monday; //星期可以随便赋值，项目不用星期

	RTC_TimeStruct.RTC_Hours   = t->hour;
	RTC_TimeStruct.RTC_Minutes = t->min;
	RTC_TimeStruct.RTC_Seconds= t->sec;
	RTC_TimeStruct.RTC_H12     = RTC_H12_PM;	//24小时制！！

	PWR_BackupAccessCmd(ENABLE);
	RTC_SetDate(RTC_Format_BIN,&RTC_DateStruct);	//二进制格式写入日期
	RTC_SetTime(RTC_Format_BIN,&RTC_TimeStruct);	//二进制格式写入时间

}

/**
 * @brief 读取当前RTC时间，读到结构体RtcTime_t里面
 * @param t 输出时间结构体，上层flash离线日志直接使用！
 */
void MyRTC_ReadTime(RtcTime_t *t)
{
	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;

	RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);

	t->year = RTC_DateStruct.RTC_Year;
	t->month  = RTC_DateStruct.RTC_Month;
	t->day  = RTC_DateStruct.RTC_Date;
	t->hour = RTC_TimeStruct.RTC_Hours;
	t->min  = RTC_TimeStruct.RTC_Minutes;
	t->sec  = RTC_TimeStruct.RTC_Seconds;
}
