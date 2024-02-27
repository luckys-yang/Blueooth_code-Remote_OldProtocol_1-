#ifndef __BSP_KEY_H
#define __BSP_KEY_H

/* Public define==========================================================*/
// 信息打印(不需要调试时把宏后面去掉即可需要则把右边的部分加到下面) LOG_I(__VA_ARGS__)
#define LOG_I_Bsp_Key(...) LOG_I(__VA_ARGS__)

/*读取按键电平状态*/
#define _ReadPin_KEY_ROW0() io_read_pin(GPIO_PIN_INPUT_KEY0)         // 读取行0电平状态
#define _ReadPin_KEY_ROW1() io_read_pin(GPIO_PIN_INPUT_KEY1)         // 读取行1电平状态
#define _ReadPin_KEY_ROW2() io_read_pin(GPIO_PIN_INPUT_KEY2)           // 读取行2电平状态

#define Key_press 0 // 按键按下
#define Key_Release 1  // 按键释放

#define KEY_DEFAULT_LEVEL PIN_SET   // 按键默认电平（看设置为上拉还是下拉）

#define KEY_LONGPREES_TIME 	200 // 长按时间设置
#define KEY_TIMEOUT 200 // 按键超时时间(即在这个时间内按下几次)	

/*矩阵按键列 的数量枚举*/
typedef enum
{
    COL_0 = 0,
    COL_1 = 1
} Key_ColNumber_et;

/*按键ID类型枚举*/
typedef enum
{
    KEY_ID_UP = 14, // 上键
    KEY_ID_DOWN = 18,   // 下键
    KEY_ID_LEFT = 15,   // 左键
    KEY_ID_RIGHT = 17,  // 右键
    KEY_ID_MIDDLE = 16, // 中间键
    KEY_ID_PHOTO = 19   // 拍照键
} Key_IdType_et;

/*按键动作类型枚举*/
typedef enum
{
    Key_Click_Long = 3,   // 长按
    Key_Click_LongFinish = 4,   // 长按完成
    Key_Click_One = 11,   // 单击
    Key_Click_Two = 12,   // 双击
    Key_Click_Three = 13, // 三击
} Key_ActionType_et;

/*按键事件类型枚举*/
typedef enum
{
    EVENT_KEY_NONE = 0,              // 无操作
    EVENT_KEY_CHECK_START = 1,       // 按键检查开始事件
    EVENT_KEY_PRESS_CLEAR_SHAKE = 2, // 按下消抖事件
    EVENT_KEY_UNDO_CLEAR_SHAKE = 3,  // 松开消抖事件
    EVENT_KEY_LONG_PRESS = 4,        // 长按事件
} Key_EventType_et;

/*单个按键信息*/
typedef struct
{
    uint8_t start_signal;      // 开始检测信号(默认 Key_Release)
    uint8_t old_level;         // 上一次稳定时电平(默认 Key_Release)
    uint8_t press_count;       // 按下个数
    uint16_t long_press_time;   // 长按时间
    uint16_t check_time;        // 检测时间
    void (*key_event_handler)(Key_ActionType_et); // 按键事件处理函数指针
    void (*key_longpress_recover_handler)(void);    // 按键长按恢复处理函数指针
} KeyInfo_st;

/*按键结构体*/
typedef struct
{
    Key_ColNumber_et current_enable_col_Status;  // 当前正在使能的列(即1列输出低电平其他列输出高电平)
    PinState_et current_level_input_row0_Status;   // 当前Row0输入电平状态
    PinState_et current_level_input_row1_Status;   // 当前Row1输入电平状态
    PinState_et current_level_input_row2_Status;   // 当前Row2输入电平状态

    void (*Bsp_Key_Init)(void);                           // 按键初始化
    void (*Bsp_key_Timer_Init)(void);                     // 按键软件定时器初始化

} Bsp_Key_st;

extern Bsp_Key_st Bsp_Key;
extern struct builtin_timer *key_timer_inst;

#endif