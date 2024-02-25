#ifndef __BSP_SYSTIMER_H
#define __BSP_SYSTIMER_H

/*系统软件定时器特殊事件枚举*/
typedef enum
{
    EVENT_SoftwareReset = 0,     // 软件复位事件
} Bsp_Timer_Event_et;

/*系统软件定时器计数结构体*/
typedef struct
{
    uint16_t led1_count;  // LED1闪烁系统软件定时器计数器
    uint32_t sys_count;   // 系统软件定时器计数器
    uint16_t reset_count; // 复位软件定时器计数器
} Bsp_SysTimerCount_st;

typedef struct
{
    void (*Bsp_SysTimer_Init)(void);
} Bsp_SysTimer_st;

extern Bsp_SysTimerCount_st Bsp_SysTimerCount;
extern Bsp_SysTimer_st Bsp_SysTimer;

#endif
