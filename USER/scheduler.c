#include "scheduler.h"
#include "stm32f4xx.h"
#include <stddef.h>



static scheduler_task_t s_task_list[MAX_TASK_NUM];
static uint8_t  s_task_cnt;

static uint8_t  event_buf[EVENT_QUEUE_SIZE];
static uint8_t  event_wr, event_rd;

static volatile uint32_t g_systick_tick = 0;


/* ============================================================
 * 用 TIM6 产生 1ms 中断给调度器计时
 * 不和 delay 的 SysTick 打架，最干净的做法
 * ============================================================ */
static void scheduler_tick_timer_init(void)
{
    TIM_TimeBaseInitTypeDef t;
    NVIC_InitTypeDef        n;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

    TIM_TimeBaseStructInit(&t);
    t.TIM_Prescaler     = SystemCoreClock / 1000000 - 1;  /* 计数频率 1MHz */
    t.TIM_Period        = 1000 - 1;                        /* 1ms 溢出一次 */
    t.TIM_CounterMode   = TIM_CounterMode_Up;
    t.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM6, &t);

    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM6, ENABLE);

    n.NVIC_IRQChannel                   = TIM6_DAC_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 2;
    n.NVIC_IRQChannelSubPriority        = 0;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);
}

/* TIM6 中断服务函数，每 1ms 调一次 */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
        g_systick_tick++;
    }
}


uint32_t SysTick_GetTick(void)
{
    return g_systick_tick;
}


void scheduler_init(void)
{
    scheduler_tick_timer_init();   /* 关键：把 tick 定时器打开 */

    s_task_cnt = 0;
    for (int i = 0; i < MAX_TASK_NUM; i++)
    {
        s_task_list[i].task      = NULL;
        s_task_list[i].period    = 0;
        s_task_list[i].last_tick = 0;
    }
}


int scheduler_add_task(task_func_t func, uint32_t period_ms)
{
    if (s_task_cnt >= MAX_TASK_NUM) return -1;

    s_task_list[s_task_cnt].task      = func;
    s_task_list[s_task_cnt].period    = period_ms;

    /* ?? 减一个 period，让任务第一次进 scheduler_run 就立即执行一次
       方便调试观察，之后按 period 正常周期跑 */
    s_task_list[s_task_cnt].last_tick = SysTick_GetTick() - period_ms;

    s_task_cnt++;
    return 0;
}


void scheduler_run(void)
{
    uint32_t now;

    while (1)
    {
        now = SysTick_GetTick();

        for (int i = 0; i < s_task_cnt; i++)
        {
            if (s_task_list[i].task == NULL) continue;

            if (s_task_list[i].period == 0)
            {
                s_task_list[i].task();
            }
            else
            {
                if ((now - s_task_list[i].last_tick) >= s_task_list[i].period)
                {
                    s_task_list[i].task();
                    s_task_list[i].last_tick = now;
                }
            }
        }

        /* 全部任务执行完，CPU 轻度休眠，等 TIM6 中断唤醒 */
        __WFI();
    }
}


void event_init(void)
{
    event_wr = 0;
    event_rd = 0;
}

void event_post(uint8_t evt)
{
    uint8_t next = (event_wr + 1) % EVENT_QUEUE_SIZE;
    if (next != event_rd)
    {
        event_buf[event_wr] = evt;
        event_wr = next;
    }
}

uint8_t event_get(void)
{
    if (event_rd == event_wr) return 0xff;
    uint8_t ret = event_buf[event_rd];
    event_rd = (event_rd + 1) % EVENT_QUEUE_SIZE;
    return ret;
}
