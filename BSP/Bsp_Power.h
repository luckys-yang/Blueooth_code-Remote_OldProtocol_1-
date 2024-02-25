#ifndef __BSP_POWER_H
#define __BSP_POWER_H

/* Public define==========================================================*/
// 信息打印
#define LOG_I_Bsp_Power(...) LOG_I(__VA_ARGS__)

typedef struct
{
    void (*Bsp_Power_Enter_LowPowerMode)(void); // 进入低功耗模式
} Bsp_Power_st;

extern Bsp_Power_st Bsp_Power;
#endif
