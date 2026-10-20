#ifndef __SCHEDULER_H
#define __SCHEDULER_H
#include "stm32f4xx.h"

#define MAX_TASK_NUM	8
#define EVENT_QUEUE_SIZE	16

typedef void (*task_func_t)(void);

typedef struct
{
	task_func_t task;
	uint32_t period;		//执行周期ms；0表示就绪立刻执行
	uint32_t last_tick;
}scheduler_task_t;

uint32_t SysTick_GetTick(void);
void SysTick_Handler(void);
void scheduler_init(void);
int scheduler_add_task(task_func_t func,uint32_t period_ms);
void scheduler_run(void);

//事件
void event_init(void);
void event_post(uint8_t evt);
uint8_t event_get(void);

#endif
