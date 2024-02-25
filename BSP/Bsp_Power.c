/***************************************************************************
 * File: Bsp_Power.c
 * Author: Yang
 * Date: 2024-02-07 15:47:59
 * description:
 -----------------------------------
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_Power_Enter_LowPowerMode(void);

/* Public variables==========================================================*/

Bsp_Power_st Bsp_Power =
{
    .Bsp_Power_Enter_LowPowerMode = &Bsp_Power_Enter_LowPowerMode
};

/**
* @param    None
* @retval   None
* @brief    进入低功耗模式
**/
static void Bsp_Power_Enter_LowPowerMode(void)
{
    System_Status.low_power_mode = FLAG_true;
    /*LED全部灭*/
    Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, 0);
    Bsp_Led.Bsp_Led_All_Close();
    builtin_timer_stop(key_timer_inst);

    /*都输出为低电平*/
    io_write_pin(GPIO_PIN_OUTPUT_KEY0, PIN_RESET);
    io_write_pin(GPIO_PIN_OUTPUT_KEY1, PIN_RESET);

    struct deep_sleep_wakeup wakeup;
    memset(&wakeup, 0, sizeof(wakeup));
    wakeup.pa07 = 1;
    wakeup.pa07_rising_edge = 0; // 1 为上升沿唤醒，0 为下降沿唤醒；
    wakeup.pa00 = 1;
    wakeup.pa00_rising_edge = 0;
    wakeup.pb15 = 1;
    wakeup.pb15_rising_edge = 0;
    enter_deep_sleep_mode_lvl2_lvl3(&wakeup); // 调用睡眠函数
}