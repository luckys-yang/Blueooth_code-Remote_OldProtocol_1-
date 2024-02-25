/***************************************************************************
 * File: Bsp_Uart.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
串口：
    uart1为云台串口，uart3为上位机串口，串口2为2.4g接收(遥控器+AI)
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Public variables==========================================================*/

static void Bsp_Uart_Ble_SendData(uint8_t *data, uint16_t length);
static void Bsp_Uart_RecData_AddPosition(uint16_t *count, uint16_t add_num);
static void Bsp_Uart_ParameterInit(void);
/* Public variables==========================================================*/
uint8_t ble_rx_buffer[BLE_SVC_BUFFER_SIZE]; // ble接收缓存数组
uint8_t ble_tx_buffer[BLE_SVC_BUFFER_SIZE]; // ble发送缓存数组

Bsp_Uart_st Bsp_Uart =
{
    .UartInnfo =
    {
        {
            .uart_rx_buffer_ptr = ble_rx_buffer,
            .uart_tx_buffer_ptr = ble_tx_buffer,
            .uart_rx_index = 0,
            .uart_tx_index = 0,
            .uart_tx_busy_Flag = FLAG_true,
            .uart_rx_busy_Flag = FLAG_true
        }     // 蓝牙串口
    },

    .Bsp_Uart_Ble_SendData = &Bsp_Uart_Ble_SendData,
    .Bsp_Uart_RecData_AddPosition = &Bsp_Uart_RecData_AddPosition,
    .Bsp_Uart_ParameterInit = &Bsp_Uart_ParameterInit
};
/*--------------------串口队列解析实例------------------------*/
// 蓝牙队列数据解析结构体变量
Uart_QueueParse_st Uart_QueueParse_Ble =
{
    .deal_queue = {0},
    .deal_queue_index = 0,
    .Rec_Buffer_ptr = ble_rx_buffer
};

/**
* @param    None
* @retval   None
* @brief    串口参数初始化
**/
static void Bsp_Uart_ParameterInit(void)
{
    /*
    注意不能在这里进行数组或者别的耗时初始化否则会导致蓝牙断开！！！
    不初始化发送索引，否则会导致切换协议触发不会回复确认包
    */
    /*串口信息结构体初始化*/
    for (uint8_t i = 0; i < UART_MAX; i++)
    {
        /*另外两个数组指针不初始化了，暂时没问题*/
        Bsp_Uart.UartInnfo[i].uart_rx_index = 0;
        Bsp_Uart.UartInnfo[i].uart_tx_busy_Flag = FLAG_true;
        Bsp_Uart.UartInnfo[i].uart_rx_busy_Flag = FLAG_true;
    }
    /*串口队列数据解析结构体初始化*/
    // 不能初始化Uart_QueueParse_Uart1的否则也会卡死断开连接
}

/**
 * @param    data -> 需要发送数据的地址
 * @param    length -> 需要发送数据长度
 * @retval   None
 * @brief    蓝牙发送数据函数
 **/
static void Bsp_Uart_Ble_SendData(uint8_t *data, uint16_t length)
{
    if (FLAG_true == System_Status.bluetooth)
    {
        LS_ASSERT((length + Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index) <= BLE_SVC_BUFFER_SIZE);            // 使用 LS_ASSERT 宏进行断言检查，确保发送的数据长度不超过 BLE_SVC_BUFFER_SIZE
        memcpy(&ble_tx_buffer[Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index], (uint8_t *)data, length); // 将 data 指向的数据拷贝到 ble_tx_buffer 中
        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index += length; // 更新发送索引，指向下一个可用位置
    }
}

/**
 * @param    count -> 接收缓冲协议解析索引
 * @param    add_num -> 增加大小
 * @retval   CRC结果
 * @brief    对接收缓冲协议解析索引值进行增加
 **/
static void Bsp_Uart_RecData_AddPosition(uint16_t *count, uint16_t add_num)
{
    if (((*count) + add_num) >= Rec_Buffer_size) // 超出最大值(合理范围正常是: 0 ~ Rec_Buffer_size - 1)
    {
        /*
        得到超过缓冲区大小的差值 目的是实现一个循环缓冲区的效果
        当计数器超过缓冲区大小时，将其调整为剩余的位置，从而实现数据在缓冲区中的循环存储
        */
        (*count) = (*count) + add_num - Rec_Buffer_size;
    }
    else
    {
        (*count) += add_num;
    }
}