#ifndef __BSP_LED_H
#define __BSP_LED_H

/* Public define==========================================================*/
/*LED控制亮灭 亮：PIN_RESET 灭：PIN_SET*/
#define _Led1_Conrol(x) io_write_pin(GPIO_PIN_LED1, x)

#define Led_Blink_Cnt_Neverending 0xFF  // 一直闪烁不会停止标识数值
#define Led_Blink_Cnt_BleConnectBlink 0x06  // 蓝牙连接时闪烁次数
#define Led_Blink_Cnt_KeyPressBlink 0x02    // 按键按下时闪烁次数


/*LED编号枚举*/
typedef enum
{
    LED_1 = 0,
    LED_MAX // 最大led数(用作后面定义数组大小)
} Bsp_LedNum_st;

/*LED状态枚举*/
typedef enum
{
    no_blink = 0x00, // 不闪烁
    blink,    // 闪烁
    ble_connect_blink,  // 蓝牙连接时的闪烁
    key_press_blink // 按键按下时的闪烁
} Bssp_LedStatus_et;

/*LED信息结构体*/
typedef struct
{
    uint8_t led_status; // led状态
    uint16_t led_flip_timeout;    // led翻转时间间隔
    uint16_t led_blink_count;   // led闪烁次数
} LedInfo_st;

/*LED结构体*/
typedef struct
{
    LedInfo_st LedInfo[LED_MAX];    // 定义led信息结构体变量

    void (*Bsp_Led_Init)(void);                                                   // LED初始化
    void (*Bsp_Led_BlinkConrol_Handler)(Bsp_LedNum_st, Bssp_LedStatus_et, uint16_t); // LED闪烁控制处理函数
    void (*Bsp_Led_All_Close)(void);                                              // LED全灭
    void (*Bsp_Led_StatusUpdate_Handler)(void);                                   // LED状态刷新处理
} Bsp_Led_st;

extern Bsp_Led_st Bsp_Led;

#endif
