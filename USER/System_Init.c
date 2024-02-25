/***************************************************************************
 * File: System_Init.c
 * Author: Yang
 * Date: 2024-02-04 14:56:54
 * description:
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private variables=========================================================*/
// 存储按键板编译时间数组(年 月 日 时 分 秒 秒)
static uint8_t device_compile_time[16] = "20240220195151";

/* Private function prototypes===============================================*/


/* Public function prototypes=========================================================*/

static void Hardware_Init(void);

/* Public variables==========================================================*/
System_Status_st System_Status =
{
    .update_mode = FLAG_false,
    .bluetooth = FLAG_false,
    .sys_power_switch = FLAG_false,
    .sys_timer_signal = FLAG_false,
    .low_power_mode = FLAG_false
};

System_KeyBoardInfo_st System_KeyBoardInfo =
{
    .device_compile_time_ptr = device_compile_time
};

System_Init_st System_Init =
{
    .Hardware_Init = &Hardware_Init
};

/**
 * @param    None
 * @retval   None
 * @brief    硬件初始化
 **/
static void Hardware_Init(void)
{
    sys_init_app();                 // 【协议栈FUNC】系统初始化
    SEGGER_RTT_Init();              // 初始化RTT控制块(用于J-Link调试打印，生产时去掉即可)
    Bsp_Led.Bsp_Led_Init();         // LED初始化
    Bsp_Key.Bsp_Key_Init();         // 按键初始化
    ble_init();                     // BLE 初始化
    System_Status.sys_power_switch = FLAG_true; // 标志位置1
    Bsp_BlueTooth.Bsp_BlueTooth_Init();       // 蓝牙相关初始化
}