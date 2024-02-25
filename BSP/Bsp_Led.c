/***************************************************************************
 * File: xxx.c
 * Author: Yang
 * Date: 2024-02-25 14:19:35
 * description:
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_Led_Init(void);
static void Bsp_Led_BlinkConrol_Handler(Bsp_LedNum_st led_numbe, Bssp_LedStatus_et led_status, uint16_t timeout);
static void Bsp_Led_All_Close(void);
static void Bsp_Led_StatusUpdate_Handler(void);
/* Public variables==========================================================*/
Bsp_Led_st Bsp_Led =
{
    .LedInfo =
    {
        {
            .led_status = no_blink,
            .led_flip_timeout = 0,
            .led_blink_count = 0
        }
    },
    .Bsp_Led_Init = &Bsp_Led_Init,
    .Bsp_Led_BlinkConrol_Handler = &Bsp_Led_BlinkConrol_Handler,
    .Bsp_Led_All_Close = &Bsp_Led_All_Close,
    .Bsp_Led_StatusUpdate_Handler = &Bsp_Led_StatusUpdate_Handler,
};

/**
 * @param    None
 * @retval   None
 * @brief    LED初始化
 **/
static void Bsp_Led_Init(void)
{
    io_cfg_output(GPIO_PIN_LED1); // 输出模式
    _Led1_Conrol(PIN_SET);      // 默认灭
    Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, BLE_DISCONNECT_BLINK_TIME);
}

/**
 * @param    led_number - LED编号 @Bsp_LedNum_st
 * @param    led_status - LED状态(闪烁/不闪烁) @Bssp_LedStatus_et
 * @param    timeout - 间隔时间
 * @retval   None
 * @brief    LED闪烁控制处理函数
 **/
static void Bsp_Led_BlinkConrol_Handler(Bsp_LedNum_st led_numbe, Bssp_LedStatus_et led_status, uint16_t timeout)
{
    switch (led_status)
    {
    case no_blink:
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = no_blink;
            Bsp_SysTimerCount.led1_count = 0; // 将系统软件定时器LED1闪烁计数器置0
            _Led1_Conrol(PIN_SET);
            break;
        }
        default:
            break;
        }
        break;
    }
    case blink:
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = blink;
            Bsp_Led.LedInfo[LED_1].led_flip_timeout = timeout;
            Bsp_Led.LedInfo[LED_1].led_blink_count = Led_Blink_Cnt_Neverending;
            Bsp_SysTimerCount.led1_count = 0; // 将系统软件定时器LED1闪烁计数器置0
            break;
        }
        default:
            break;
        }
        break;
    }
    case ble_connect_blink:
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = ble_connect_blink;
            Bsp_Led.LedInfo[LED_1].led_flip_timeout = timeout;
            Bsp_Led.LedInfo[LED_1].led_blink_count = Led_Blink_Cnt_BleConnectBlink;
            Bsp_SysTimerCount.led1_count = 0; // 将系统软件定时器LED1闪烁计数器置0
            break;
        }
        default:
            break;
        }
        break;
    }
    case key_press_blink:
    {
        switch (led_numbe)
        {
        case LED_1:
        {
            Bsp_Led.LedInfo[LED_1].led_status = key_press_blink;
            Bsp_Led.LedInfo[LED_1].led_flip_timeout = timeout;
            Bsp_Led.LedInfo[LED_1].led_blink_count = Led_Blink_Cnt_KeyPressBlink;
            Bsp_SysTimerCount.led1_count = 0; // 将系统软件定时器LED1闪烁计数器置0
            _Led1_Conrol(PIN_SET);
            break;
        }
        default:
            break;
        }
        break;
    }
    }
}

/**
 * @param    None
 * @retval   None
 * @brief    全灭
 **/
static void Bsp_Led_All_Close(void)
{
    Bsp_Led.LedInfo[LED_1].led_status = no_blink;
    _Led1_Conrol(PIN_SET);
}


/**
 * @param    None
 * @retval   None
 * @brief    LED状态刷新处理
 **/
static void Bsp_Led_StatusUpdate_Handler(void)
{
    /*------------------LED1--------------------------*/
    if (Bsp_Led.LedInfo[LED_1].led_status != no_blink) // 若LED状态为闪烁
    {
        if ((Bsp_SysTimerCount.led1_count * SYS_TIME) > Bsp_Led.LedInfo[LED_1].led_flip_timeout) // 到达闪烁间隔
        {
            if (Bsp_Led.LedInfo[LED_1].led_blink_count != 0)    // 闪烁次数不为0则
            {
                io_toggle_pin(GPIO_PIN_LED1);  // 翻转LED

                if (Bsp_Led.LedInfo[LED_1].led_blink_count != Led_Blink_Cnt_Neverending)    // 不是一直闪烁的则需要进行闪烁次数--
                {
                    Bsp_Led.LedInfo[LED_1].led_blink_count--;

                    if (0 == Bsp_Led.LedInfo[LED_1].led_blink_count)
                    {
                        _Led1_Conrol(PIN_SET);
                    }
                }
            }
            Bsp_SysTimerCount.led1_count = 0; // 系统软件定时器LED闪烁计数器清0
        }
        else
        {
            Bsp_SysTimerCount.led1_count++; // 若没有到达闪烁时间，系统软件定时器LED闪烁计数器加1
        }
    }
}