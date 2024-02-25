/***************************************************************************
 * File: Bsp_SysTimer.c
 * Author: Yang
 * Date: 2024-02-04 20:44:21
 * description:
 -----------------------------------
None
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public function prototypes=========================================================*/

static void Bsp_SysTimer_Init(void);
static void Bsp_SysTimer_CallBack(void *arg);

/* Private function prototypes===============================================*/

static void vSysTimerEvent_SerialPort_SendData_Handler(void);
static void vSysTimerEvent_LowPowerMode_Handler(void);
static void vSysTimerEvent_SoftwareReset_Handler(void);
/* Public variables==========================================================*/
struct builtin_timer *sys_timer_inst = NULL;    // 系统软件定时器句柄
// 模块超时时间结构体
Bsp_SysTimerCount_st Bsp_SysTimerCount =
{
    .led1_count = 0,
    .sys_count = 0,
    .reset_count = 0,
};

Bsp_SysTimer_st Bsp_SysTimer =
{
    .Bsp_SysTimer_Init = &Bsp_SysTimer_Init
};

/**
 * @param    None
 * @retval   None
 * @brief    系统软件定时器初始化
 **/
static void Bsp_SysTimer_Init(void)
{
    sys_timer_inst = builtin_timer_create(Bsp_SysTimer_CallBack); // 创建软件定时器 参数-回调函数指针
    builtin_timer_start(sys_timer_inst, SYS_TIME, NULL);          // 启动定时器 参数1-指向定时器的指针 参数2-定时器的超时时间 参数3-传递给定时器回调函数的参数
}

/**
 * @param    None
 * @retval   None
 * @brief    系统软件定时器回调函数
 **/
static void Bsp_SysTimer_CallBack(void *arg)
{
    vSysTimerEvent_SoftwareReset_Handler();       // 定时器事件--软件复位处理
    vSysTimerEvent_SerialPort_SendData_Handler(); // 定时器事件--检测串口发送缓冲区是否有数据有则发送事件处理

    vSysTimerEvent_LowPowerMode_Handler();              // 定时器事件--低功耗模式处理

    Bsp_Led.Bsp_Led_StatusUpdate_Handler();              // LED状态刷新
    builtin_timer_start(sys_timer_inst, SYS_TIME, NULL); // 重新启动定时器
}

/**
 * @param    None
 * @retval   None
 * @brief    定时器事件--检测串口发送缓冲区是否有数据有则发送事件处理
 **/
static void vSysTimerEvent_SerialPort_SendData_Handler(void)
{
    if (Bsp_BlueTooth.ble_connect_id != BLE_DISCONNECTED_ID)   // 蓝牙发送通知
    {
        uint32_t cpu_stat = enter_critical();
        Bsp_BlueTooth.Bsp_BlueTooth_Send_Notification();
        exit_critical(cpu_stat);
    }
}

/**
* @param    None
* @retval   None
* @brief    定时器事件--低功耗模式处理
**/
static void vSysTimerEvent_LowPowerMode_Handler(void)
{
    // LOW_POWER_TIME * 60 * 1000
    if ((Bsp_SysTimerCount.sys_count >= (7000)) && (System_Status.low_power_mode != FLAG_true))
    {
        Bsp_SysTimerCount.sys_count = 0;
        Bsp_Power.Bsp_Power_Enter_LowPowerMode();
    }
    else if (System_Status.low_power_mode != FLAG_true)
    {
        Bsp_SysTimerCount.sys_count++;
    }
}

/**
* @param    None
* @retval   None
* @brief    定时器事件--软件复位处理
**/
static void vSysTimerEvent_SoftwareReset_Handler(void)
{
    if (1 == _get_bit_value(System_Status.sys_timer_signal, EVENT_SoftwareReset))
    {
        if (Bsp_SysTimerCount.reset_count * SYS_TIME >= SOFTWARE_RESET_TIME)
        {
            Bsp_SysTimerCount.reset_count = 0;
            Bsp_Boot.Bsp_Boot_InfoGet(&Current_BootInfo);    // 获取映像信息页
            ota_boot_addr_set(Current_BootInfo->Image_Base); // 设置当前映像地址
            platform_reset(0);                               // 芯片复位

            _clear_bit_value(System_Status.sys_timer_signal, EVENT_SoftwareReset);
        }
        else
        {
            Bsp_SysTimerCount.reset_count++;
        }
    }
}