/***************************************************************************
 * File: Bsp_Key.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
按键检测逻辑实现：

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void Key_Up_Press_Handler(Key_ActionType_et action_type);
static void Key_Down_Press_Handler(Key_ActionType_et action_type);
static void Key_Left_Press_Handler(Key_ActionType_et action_type);
static void Key_Right_Press_Handler(Key_ActionType_et action_type);
static void Key_Middle_Press_Handler(Key_ActionType_et action_type);
static void Key_Photo_Press_Handler(Key_ActionType_et action_type);
static void Key_Up_LongPress_Recover_Handler(void);
static void Key_Down_LongPress_Recover_Handler(void);
static void Key_Left_LongPress_Recover_Handler(void);
static void Key_Right_LongPress_Recover_Handler(void);
static void Key_Middle_LongPress_Recover_Handler(void);
static void Key_Photo_LongPress_Recover_Handler(void);

static void KeyTimer_CallBack(void *arg);
static void Key_Input_Scan(void);
static void Key_Output_Scan(void);
static uint8_t Key_Event_Scan(KeyInfo_st *keyInfo, PinState_et pin_status);
/* Public function prototypes=========================================================*/

static void Bsp_Key_Init(void);
static void Bsp_key_Timer_Init(void);
/* Public variables==========================================================*/
struct builtin_timer *key_timer_inst = NULL;   // 按键服务软件定时器句柄
/*按键信息结构体变量*/
KeyInfo_st KeyInfo_up =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Up_Press_Handler,
    .key_longpress_recover_handler = &Key_Up_LongPress_Recover_Handler
};
KeyInfo_st KeyInfo_Down =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Down_Press_Handler,
    .key_longpress_recover_handler = &Key_Down_LongPress_Recover_Handler
};
KeyInfo_st KeyInfo_Left =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Left_Press_Handler,
    .key_longpress_recover_handler = &Key_Left_LongPress_Recover_Handler
};
KeyInfo_st KeyInfo_Right =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Right_Press_Handler,
    .key_longpress_recover_handler = &Key_Right_LongPress_Recover_Handler
};
KeyInfo_st KeyInfo_Middle =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Middle_Press_Handler,
    .key_longpress_recover_handler = &Key_Middle_LongPress_Recover_Handler
};
KeyInfo_st KeyInfo_Photo =
{
    .start_signal = EVENT_KEY_NONE,
    .old_level = Key_Release,
    .press_count = 0,
    .long_press_time = 0,
    .check_time = 0,
    .key_event_handler = &Key_Photo_Press_Handler,
    .key_longpress_recover_handler = &Key_Photo_LongPress_Recover_Handler
};

/*按键结构体变量*/
Bsp_Key_st Bsp_Key =
{
    .current_enable_col_Status = COL_0,
    .current_level_input_row0_Status = KEY_DEFAULT_LEVEL,
    .current_level_input_row1_Status = KEY_DEFAULT_LEVEL,
    .current_level_input_row2_Status = KEY_DEFAULT_LEVEL,

    .Bsp_Key_Init = &Bsp_Key_Init,
    .Bsp_key_Timer_Init = &Bsp_key_Timer_Init
};


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★按键应用层处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    None
 * @retval   None
 * @brief    按键初始化
 **/
static void Bsp_Key_Init(void)
{
    /*列设置为输出模式*/
    io_cfg_output(GPIO_PIN_OUTPUT_KEY0);
    io_cfg_output(GPIO_PIN_OUTPUT_KEY1);
    /*行设置为输入模式*/
    io_cfg_input(GPIO_PIN_INPUT_KEY0);   // 配置IO口为输入
    io_pull_write(GPIO_PIN_INPUT_KEY0, IO_PULL_UP);  // 配置IO口为内部上拉
    io_cfg_input(GPIO_PIN_INPUT_KEY1);   // 配置IO口为输入
    io_pull_write(GPIO_PIN_INPUT_KEY1, IO_PULL_UP);  // 配置IO口为内部上拉
    io_cfg_input(GPIO_PIN_INPUT_KEY2);   // 配置IO口为输入
    io_pull_write(GPIO_PIN_INPUT_KEY2, IO_PULL_UP);  // 配置IO口为内部上拉

    /*列默认输出高电平*/
    io_write_pin(GPIO_PIN_OUTPUT_KEY0, PIN_SET);
    io_write_pin(GPIO_PIN_OUTPUT_KEY1, PIN_SET);
}

/**
 * @param    None
 * @retval   None
 * @brief    按键软件定时器初始化
 **/
static void Bsp_key_Timer_Init(void)
{
    key_timer_inst = builtin_timer_create(KeyTimer_CallBack);
    builtin_timer_start(key_timer_inst, KEY_CHECK_TIME, NULL);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★按键功能处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    上键按下处理函数
**/
static void Key_Up_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_UP;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    下键按下处理函数
**/
static void Key_Down_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_DOWN;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    左键按下处理函数
**/
static void Key_Left_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_LEFT;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    右键按下处理函数
**/
static void Key_Right_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_RIGHT;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    中间键按下处理函数
**/
static void Key_Middle_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_MIDDLE;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    action_type -> 动作事件类型 见@Key_ActionType_et
* @retval   None
* @brief    拍照键按下处理函数
**/
static void Key_Photo_Press_Handler(Key_ActionType_et action_type)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_PHOTO;
    key_data[1] = action_type;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
}

/**
* @param    None
* @retval   None
* @brief    上键长按恢复处理函数
**/
static void Key_Up_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_UP;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}

/**
* @param    None
* @retval   None
* @brief    下键长按恢复处理函数
**/
static void Key_Down_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_DOWN;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}

/**
* @param    None
* @retval   None
* @brief    左键长按恢复处理函数
**/
static void Key_Left_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_LEFT;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}

/**
* @param    None
* @retval   None
* @brief    右键长按恢复处理函数
**/
static void Key_Right_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_RIGHT;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}

/**
* @param    None
* @retval   None
* @brief    中间键长按恢复处理函数
**/
static void Key_Middle_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_MIDDLE;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}

/**
* @param    None
* @retval   None
* @brief    拍照键长按恢复处理函数
**/
static void Key_Photo_LongPress_Recover_Handler(void)
{
    uint8_t key_data[2];
    key_data[0] = KEY_ID_PHOTO;
    key_data[1] = Key_Click_LongFinish;
    Bsp_OldProtocol.Bsp_OldProtocol_SendPackage(OLD_ADDR_KEY_BOARD, Old_Protocol_CMD_ExendKey, 2, key_data);
    _Led1_Conrol(PIN_SET);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★按键底层处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/**
 * @param    arg -> 开启软件定时器时传入的参数
 * @retval   None
 * @brief    按键软件定时器回调函数
 **/
static void KeyTimer_CallBack(void *arg)
{
    if (FLAG_true == System_Status.bluetooth) // 蓝牙状态下才进行
    {
        Key_Output_Scan(); // 输出检测
        Key_Input_Scan();  // 输入检测
    }
    builtin_timer_start(key_timer_inst, KEY_CHECK_TIME, NULL);
}

/**
* @param    None
* @retval   None
* @brief    按键输入检测
**/
static void Key_Input_Scan(void)
{
    /*读取所有行的输入电平状态*/
    Bsp_Key.current_level_input_row0_Status = _ReadPin_KEY_ROW0();
    Bsp_Key.current_level_input_row1_Status = _ReadPin_KEY_ROW1();
    Bsp_Key.current_level_input_row2_Status = _ReadPin_KEY_ROW2();

    switch (Bsp_Key.current_enable_col_Status)
    {
    case COL_0: // 当前是检测第0列
    {
        /*第0列对应可以检测的按键是：上 中 下*/
        Key_Event_Scan(&KeyInfo_up, Bsp_Key.current_level_input_row0_Status);
        Key_Event_Scan(&KeyInfo_Middle, Bsp_Key.current_level_input_row1_Status);
        Key_Event_Scan(&KeyInfo_Down, Bsp_Key.current_level_input_row2_Status);
        break;
    }
    case COL_1: // 当前是检测第1列
    {
        /*第1列对应可以检测的按键是：左 右 拍照*/
        Key_Event_Scan(&KeyInfo_Left, Bsp_Key.current_level_input_row0_Status);
        Key_Event_Scan(&KeyInfo_Right, Bsp_Key.current_level_input_row1_Status);
        Key_Event_Scan(&KeyInfo_Photo, Bsp_Key.current_level_input_row2_Status);
        break;
    }
    default:
        break;
    }
}

/**
* @param    None
* @retval   None
* @brief    按键输出检测
**/
static void Key_Output_Scan(void)
{
    switch (Bsp_Key.current_enable_col_Status)
    {
    case COL_0:
    {
        io_write_pin(GPIO_PIN_OUTPUT_KEY0, PIN_SET);
        io_write_pin(GPIO_PIN_OUTPUT_KEY1, PIN_RESET);
        Bsp_Key.current_enable_col_Status = COL_1;  // 检测第1列，其他列高电平
        break;
    }
    case COL_1:
    {
        io_write_pin(GPIO_PIN_OUTPUT_KEY0, PIN_RESET);
        io_write_pin(GPIO_PIN_OUTPUT_KEY1, PIN_SET);
        Bsp_Key.current_enable_col_Status = COL_0; // 检测第0列，其他列高电平
        break;
    }
    default:
        break;
    }
}

/**
* @param    keyInfo -> 按键信息句柄
* @param    pin_status -> 句柄对应的按键电平输入状态
* @retval   0x00:松开消抖开始 0x01:按键无操作 0x02:按键多次按下处理完成 0x03:按下按键消抖 0x04:松开按键消抖 0x05:按键长按 0x06:按下消抖开始 0x07:已到达长按时间，还在长按，不重复响应
* @brief    按键事件检测
**/
static uint8_t Key_Event_Scan(KeyInfo_st *keyInfo, PinState_et pin_status)
{
    /*------------------第一部分: 消抖事件 --------------------------*/
    if (_get_bit_value(keyInfo->start_signal, EVENT_KEY_PRESS_CLEAR_SHAKE)) // 若是按下按键消抖事件
    {
        keyInfo->check_time += KEY_INTERVAL_TIME;   // 把实际检测的时间也加上

        if (keyInfo->check_time >= KEY_CLEAR_SHAKE_TIMME)   // 若消抖时间到达
        {
            if (Key_press == pin_status)    // 若不是抖动，是按下稳定电平
            {
                Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, key_press_blink, KEY_PRESS_BLINK_TIME);  // 按下时LED进行闪烁

                keyInfo->press_count++; // 按下次数加1
                keyInfo->long_press_time = KEY_CLEAR_SHAKE_TIMME;   // 长按等于消抖时间
                keyInfo->old_level = Key_press;   // 保存此次电平值

                if (!_get_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START))  // 是第一次按下
                {
                    _set_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START);   // 按键检查开始事件置1
                }
            }
            keyInfo->check_time = 0; // 重新开启检测周期
            _clear_bit_value(keyInfo->start_signal, EVENT_KEY_PRESS_CLEAR_SHAKE);   // 清除按键消抖时间
        }
        LOG_I_Bsp_Key("1");
        return 0x03;    // 按下按键消抖
    }
    else if (_get_bit_value(keyInfo->start_signal, EVENT_KEY_UNDO_CLEAR_SHAKE))  // 若是松开按键消抖事件
    {
        keyInfo->check_time += KEY_INTERVAL_TIME;

        if (keyInfo->check_time >= KEY_CLEAR_SHAKE_TIMME)
        {
            if (Key_Release == pin_status)    // 若不是抖动，是松开稳定电平
            {
                keyInfo->old_level = Key_Release;   // 保存此次电平值

                if (_get_bit_value(keyInfo->start_signal, EVENT_KEY_LONG_PRESS))    // 如果是长按事件则 进行【长按恢复处理】
                {
                    keyInfo->key_longpress_recover_handler();
                    keyInfo->check_time = 0;    // 检测时间清0
                    _clear_bit_value(keyInfo->start_signal, EVENT_KEY_LONG_PRESS);  // 清除长按事件
                }
            }
            _clear_bit_value(keyInfo->start_signal, EVENT_KEY_UNDO_CLEAR_SHAKE);    // 清除松开消抖事件
        }
        LOG_I_Bsp_Key("2");
        return 0x04;    // 松开按键消抖
    }

    /*------------------第二部分: 处理输入电平事件 --------------------------*/
    if (Key_press == pin_status)    // 若是电平处于按下状态
    {
        if (pin_status == keyInfo->old_level)   // 在长按
        {
            if (!_get_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START))  // 若按键开始检测事件没置1
            {
                LOG_I_Bsp_Key("3");
                return 0x01;    // 按键无操作
            }

            if (_get_bit_value(keyInfo->start_signal, EVENT_KEY_LONG_PRESS))
            {
                LOG_I_Bsp_Key("4");
                return 0x07;    // 已到达长按时间，还在长按，不重复响应
            }

            keyInfo->long_press_time += KEY_INTERVAL_TIME;

            if (keyInfo->long_press_time >= KEY_LONGPREES_TIME)
            {
                keyInfo->key_event_handler(Key_Click_Long); // 【长按处理】
                _set_bit_value(keyInfo->start_signal, EVENT_KEY_LONG_PRESS);    // 长按事件置1

                keyInfo->long_press_time = 0;   // 长按时间清0
                keyInfo->press_count = 0;
                keyInfo->check_time = 0;
            }
            LOG_I_Bsp_Key("5");
            return 0x06;
        }
        else    // 一次按下事件 
        {
            _set_bit_value(keyInfo->start_signal, EVENT_KEY_PRESS_CLEAR_SHAKE);   // 按下消抖事件置位
            LOG_I_Bsp_Key("6");
            return 0x06;
        }
    }
    else // 若是电平处于松下状态
    {
        if (pin_status == keyInfo->old_level)   // 无操作
        {
            if (!_get_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START))  // 若不是按键检查开始事件
            {
                LOG_I_Bsp_Key("10");
                return 0x01;    // 按键无操作
            }
            keyInfo->check_time += KEY_INTERVAL_TIME;   // 无操作下时间增加

            if (keyInfo->check_time >= KEY_TIMEOUT) // 若达到一次按键检测周期无操作，则开始响应按键检测事件
            {
                switch (keyInfo->press_count)
                {
                case 1:
                {
                    keyInfo->key_event_handler(Key_Click_One);
                    break;
                }
                case 2:
                {
                    keyInfo->key_event_handler(Key_Click_Two);
                    break;
                }
                case 3:
                {
                    keyInfo->key_event_handler(Key_Click_Three);
                    break;
                }
                default:
                    break;
                }
                keyInfo->long_press_time = 0;   // 长按时间清0
                keyInfo->press_count = 0;   // 按下个数清0
                keyInfo->check_time = 0;    // 检测时间清0
                _clear_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START); // 清除按键检查开始事件
            }
            LOG_I_Bsp_Key("7");
            return 0x02;
        }
        else    // 一次松开事件
        {
            if (!_get_bit_value(keyInfo->start_signal, EVENT_KEY_CHECK_START))
            {
                LOG_I_Bsp_Key("8");
                return 0x01;  // 按键无操作
            }
            _set_bit_value(keyInfo->start_signal, EVENT_KEY_UNDO_CLEAR_SHAKE);  // 松开消抖事件置1
            LOG_I_Bsp_Key("9");
            return 0x00;
        }
    }
}