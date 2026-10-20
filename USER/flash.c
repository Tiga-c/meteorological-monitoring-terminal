#include "flash.h"
#include "RTC.h"
#include <stdio.h>
#include <string.h>

// VET6 ！！使用Sector7，0x08060000
#define LOG_START_ADDR  0x08060000U
#define MAX_LOG_COUNT   30

//获取已经存储日志条数
uint16_t Get_Log_Count(void)
{
    uint16_t cnt = 0;
    uint32_t addr = LOG_START_ADDR;
    SensorLogTypeDef temp_log;

    while(1)
    {
        if(addr >= LOG_START_ADDR + (128*1024))
        {
            break;
        }
        memset(&temp_log,0,sizeof(SensorLogTypeDef));   //====【改动1：每次读取前清零局部结构体，消除栈残留垃圾】====
        memcpy(&temp_log, (void *)addr, sizeof(SensorLogTypeDef));

        if(temp_log.magic != 0xAA)                      //====【改动2：依靠magic标记判断日志有效性，不再判断原始字节】====
        {
            break;
        }
        cnt ++;
        addr += sizeof(SensorLogTypeDef);
    }
    return cnt;
}

//保存一条日志
void Flash_Save_SensorLog(SensorLogTypeDef *pLog)
{
	uint16_t log_cnt = Get_Log_Count();

	if(log_cnt >= MAX_LOG_COUNT)
	{
		printf("[Flash]缓存已满！无法继续保存\r\n");
		return;
	}

	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

	uint32_t write_addr = LOG_START_ADDR + log_cnt * sizeof(SensorLogTypeDef); //====【改动3：彻底删除旧的+2头部偏移】====

	//====【改动4：你的结构体默认对齐，整体是4字节倍数，继续使用字写入Flash_ProgramWord】====
	uint32_t *pSrc32 = (uint32_t *)pLog;
	uint32_t word_num = sizeof(SensorLogTypeDef)/4;
	for(uint32_t i = 0; i < word_num; i++)
	{
		FLASH_ProgramWord(write_addr + i*4, pSrc32[i]);
	}

	FLASH_Lock();
	printf("[Flash]保存离线日志成功！log_idx=%d\r\n",log_cnt);
}

//读取第idx条日志
void Flash_Read_SensorLog(uint16_t idx, SensorLogTypeDef *pLog)
{
	uint32_t read_addr = LOG_START_ADDR + idx * sizeof(SensorLogTypeDef); //====【改动5：删除+2偏移】====
	memcpy(pLog,(void*)read_addr,sizeof(SensorLogTypeDef));    //====【改动6：使用memcpy读取，禁止直接指针解引用】====
}

//擦除日志扇区 Sector7
void Flash_Erase_LogSector(void)
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
	FLASH_EraseSector(FLASH_Sector_7, VoltageRange_3);
	FLASH_Lock();
	printf("[Flash]全部离线日志擦除完成\r\n");
}
