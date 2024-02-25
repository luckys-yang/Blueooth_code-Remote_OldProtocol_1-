#ifndef __SYSTEM_INIT_H
#define __SYSTEM_INIT_H

/* Public define==========================================================*/
// 信息打印
#define LOG_I_System_Init(...) LOG_I(__VA_ARGS__)

/* -----------------引脚定义---------------- */
/*LED引脚 (高电平亮)*/
#define GPIO_PIN_LED1 PA13    // RGB灯 --- 蓝色

/*矩阵按键引脚*/
#define GPIO_PIN_INPUT_KEY0 PA07    // 行
#define GPIO_PIN_INPUT_KEY1 PA00
#define GPIO_PIN_INPUT_KEY2 PB15

#define GPIO_PIN_OUTPUT_KEY0 PA08   // 列
#define GPIO_PIN_OUTPUT_KEY1 PA09



/* -----------------响应时间配置---------------- */
#define SOFTWARE_RESET_TIME 1000    // 切换固件后系统复位时间(ms) (建议1s，否则切换成功的回复包未发出系统就复位了)
#define SYS_TIME 1                     // 系统定时时间(ms)
#define KEY_CHECK_TIME 1          // 按键查询时间(ms)
#define KEY_INTERVAL_TIME 2 // key定时器1ms查询一次，而有两个输出，所有每个按键实际检测间隔为2ms
#define KEY_CLEAR_SHAKE_TIMME 20    // 按键消抖时间(ms)								/* 按键消抖时间 */
#define BLE_CONNECT_BLINK_TIME 200	// 蓝牙连接时LED闪烁时间(ms)
#define BLE_DISCONNECT_BLINK_TIME 500	// 蓝牙广播时LED闪烁时间(ms)
#define KEY_PRESS_BLINK_TIME 50    // 按键按下时LED闪烁时间(ms)
#define LOW_POWER_TIME 1   // 低功耗模式(分钟)


/* -----------------位操作---------------- */
#define _get_bit_value(data, offset) ((data >> offset) & 0x01)                   // 获取数据中指定位置的位值（从右往左数）
#define _set_bit_value(data, offset) (data |= (1 << offset))                     // 将数据中指定位置的位值设置为1（从右往左数）
#define _clear_bit_value(data, offset) (data &= (~(1 << offset)))                // 将数据中指定位置的位值清零（设置为0）（从右往左数）

/* 系统状态结构体 */
typedef struct
{
    uint8_t update_mode;                // 升级模式
    uint8_t bluetooth;                  // 蓝牙状态
    uint8_t sys_power_switch;           // 系统电源开关信号
    uint8_t sys_timer_signal;           // 系统定时器信号
    uint8_t low_power_mode; // 进入低功耗模式信号
} System_Status_st;

/*按键板信息存储*/
typedef struct
{
    uint8_t *device_compile_time_ptr;   // 存储按键板编译时间数组指针
} System_KeyBoardInfo_st;

typedef struct
{
    void (*Hardware_Init)(void); // 硬件初始化
} System_Init_st;

extern System_Init_st System_Init;
extern System_Status_st System_Status;
extern System_KeyBoardInfo_st System_KeyBoardInfo;
#endif
