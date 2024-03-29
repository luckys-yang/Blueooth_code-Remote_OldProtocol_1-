/***************************************************************************
 * File: Bsp_OldProtocol.c
 * Author: Yang
 * Date: 2024-02-18 21:07:14
 * description:
 -----------------------------------
旧协议解析
    旧协议(OldProtoocol)：
    【格式】:
            帧头--1Byte 固定值：0x55
            目的地址--1Byte	按键板是0x04
            源地址--1Byte
            功能码--1Byte
            用户数据长度1--1Byte
            用户数据长度2--1Byte 数据长度1和2都是一样的，所以程序里我们直接判断数据长度2即可
            用户数据(UD)--NByte	用户数据可以为空
            校验--1Byte 和校验：从目的地址开始的字节和
            举例数据帧：【确认】55 04 02 00 02 02 53 00 5D
    【传输是先低再高】
 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static uint8_t Bsp_OldProtocol_GetPackageInfo(uint8_t *Rec_Buffer, uint16_t *RecBuffer_count);
static uint8_t Bsp_OldProtocol_CRC_Calculate(uint8_t *data, uint16_t len);
static void Bsp_OldProtocol_ClearParsedPackage(Uart_QueueParse_st *deal_param, uint8_t clear_offset);
static void Bsp_OldProtocol_ParsedSuccess_Handler(uint8_t *Rec_Buffer);
static void Bsp_OldProtocol_SyntheticData_Handler(uint8_t *Rec_Buffer);
static void Bsp_OldProtocol_CmdProtocolSwich_Handler(void);


/* Public function prototypes=========================================================*/

static uint8_t Bsp_OldProtocol_RxDataParse_Handler(Uart_QueueParse_st *deal_param, uint16_t value_index, uint16_t Rec_len);
static void Bsp_OldProtocol_SendPackage(uint8_t des, uint8_t cmd, uint8_t datalen, uint8_t *data);
/* Public variables==========================================================*/
// 旧协议包解析信息结构体变量
OldProtocol_Package_Info_st OldProtocol_Package_Info = {0};

Bsp_OldProtocol_st Bsp_OldProtocol =
{
    .Bsp_OldProtocol_RxDataParse_Handler = &Bsp_OldProtocol_RxDataParse_Handler,
    .Bsp_OldProtocol_SendPackage = &Bsp_OldProtocol_SendPackage
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★旧协议应用层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    deal_param -> 串口协议解析参数结构体指针
 * @param    value_index -> 接收缓冲当前数据索引（接收当前数据前）
 * @param    Rec_len -> 接收到数据的大小
 * @retval   None
 * @brief    旧协议接收数据解析处理
 **/
static uint8_t Bsp_OldProtocol_RxDataParse_Handler(Uart_QueueParse_st *deal_param, uint16_t value_index, uint16_t Rec_len)
{
    while (Rec_len != 0)
    {
        for (uint16_t i = 0; i < deal_param->deal_queue_index; i++)
        {
            deal_param->deal_queue[i].deal_sign++; // 进度++

            if (OldProtocol_DataLen_Offset == deal_param->deal_queue[i].deal_sign) // 进度到数据长度时
            {
                deal_param->deal_queue[i].data_len = deal_param->Rec_Buffer_ptr[value_index]; // 把数据长度存储到data_len里
            }
            // 当 进度 == 数据长度+除数据外的包大小 个字节则表示一个包接收完成
            else if ((deal_param->deal_queue[i].data_len + OldProtocol_Exclude_Data_PackageLen) == deal_param->deal_queue[i].deal_sign)
            {
                // 旧协议获取数据包信息 通过协议获取对应位置信息
                if (!Bsp_OldProtocol_GetPackageInfo(deal_param->Rec_Buffer_ptr, &deal_param->deal_queue[i].count))
                {
                    Bsp_OldProtocol_ParsedSuccess_Handler(deal_param->Rec_Buffer_ptr); // 协议解析成功处理函数
                }
                Bsp_OldProtocol_ClearParsedPackage(deal_param, i); // 清除解析过的包
                i--;
            }
        }

        // 检查接收缓冲区中的数据是否是旧协议的包头，如果是则: 表示接收到了一个新的数据包
        if (OldProtocol_Package_Head == deal_param->Rec_Buffer_ptr[value_index])
        {
            deal_param->deal_queue[deal_param->deal_queue_index].deal_sign++;         // 将当前处理标志自增1，用于记录处理的进度
            deal_param->deal_queue[deal_param->deal_queue_index].count = value_index; // 将接收缓冲区的索引值赋给当前处理队列元素的计数值，以记录包头在接收缓冲区中的位置
            deal_param->deal_queue_index++;                                           // 将协议解析索引自增1，表示有一个新的处理队列元素加入
        }
        /*兼容传入的数据长度不是1时需要++*/
        value_index++;
        if (value_index == Rec_Buffer_size)
        {
            value_index = 0;
        }
        Rec_len--;
    }
    return 0x00;
}

/**
 * @param    des->发送目的地
 * @param    cmd->发送包命令@OldProtocol_CmdType_et
 * @param    datalen->发送数据长度
 * @param    data->发送数据地址
 * @retval   None
 * @brief    旧协议发送包函数
 **/
static void Bsp_OldProtocol_SendPackage(uint8_t des, uint8_t cmd, uint8_t datalen, uint8_t *data)
{
    uint8_t i;
    uint8_t crc;                          // 存储CRC校验和
    uint16_t des_Index;                   // 目标地址的下标索引
    uint8_t Rep_Buffer[Send_Buffer_size]; // 发送数据临时缓冲数组
    uint16_t RepBuffer_Index = 0;         // 发送数据临时缓冲数组下标索引

    Rep_Buffer[RepBuffer_Index] = OldProtocol_Package_Head; // 【包头】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = des; // 【目标地址】
    des_Index = RepBuffer_Index;       // 存储目标地址的下标索引
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = OLD_OWN_ADDR; // 【源地址(即按键板自己地址)】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = cmd; // 【功能码】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = datalen; // 【数据长度1】
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = datalen; // 【数据长度2】
    RepBuffer_Index++;
    // 填充数据
    for (i = 0; i < datalen; i++)
    {
        Rep_Buffer[RepBuffer_Index] = *(data + i);
        RepBuffer_Index++;
    }
    crc = Bsp_OldProtocol_CRC_Calculate(&Rep_Buffer[des_Index], datalen + 5); // 计算crc
    Rep_Buffer[RepBuffer_Index] = crc;                                        // 【crc值】
    RepBuffer_Index++;

    uint32_t cpu_stat = enter_critical();
    Bsp_Uart.Bsp_Uart_Ble_SendData(Rep_Buffer, RepBuffer_Index);
    exit_critical(cpu_stat);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★旧协议中间层部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    Rec_Buffer -> 接收数组地址
 * @param    RecBuffer_count -> 接收数组索引地址
 * @retval   0x00->成功 0x01->包数据大小大于接收缓冲大小 0x02->包数据crc校验错误
 * @brief    旧协议获取数据包信息 通过协议获取对应位置信息
 **/
static uint8_t Bsp_OldProtocol_GetPackageInfo(uint8_t *Rec_Buffer, uint16_t *RecBuffer_count)
{
    uint8_t temp_crc = 0;   // 存储计算CRC的结果临时变量
    uint16_t des_Index = 0; // 存储目标地址位置的临时变量

    OldProtocol_Package_Info.package_state = FLAG_true;          // 解析成功
    OldProtocol_Package_Info.head_position = (*RecBuffer_count); // 记录存储包头所处位置

    /*获取包目标地址--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);     // 包头位置基础上
    OldProtocol_Package_Info.des = Rec_Buffer[(*RecBuffer_count)]; // 存储包目标地址
    des_Index = (*RecBuffer_count);                                // 记录存储包目标地址所在协议中的位置
    /*获取包源地址--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.src = Rec_Buffer[(*RecBuffer_count)]; // 存储源地址
    /*获取功能码--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.cmd = Rec_Buffer[(*RecBuffer_count)]; // 存储功能码
    /*获取数据长度1--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.data_len = Rec_Buffer[(*RecBuffer_count)]; // 存储数据长度1

    if (OldProtocol_Package_Info.data_len > Rec_Buffer_size)            // 数据长度超出最大接收长度则
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // 解析失败
        return 0x01;
    }

    /*获取数据长度2--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    if (OldProtocol_Package_Info.data_len != Rec_Buffer[(*RecBuffer_count)]) // 判断数据长度2是否等于数据长度1
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // 解析失败
        return 0x01;
    }
    /*获取数据里第一个数据位置--偏移1字节*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    if (OldProtocol_Package_Info.data_len != 0) // 判断数据长度不为0
    {
        OldProtocol_Package_Info.first_data_position = (*RecBuffer_count); // 存储数据里的第一个数据的位置
    }
    /*获取CRC校验和--偏移N字节(到达一个包的最后一个字节)*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, OldProtocol_Package_Info.data_len); // 偏移多少->数据长度个大小
    OldProtocol_Package_Info.crc = Rec_Buffer[(*RecBuffer_count)];                             // 存储CRC校验和

    // 目标地址位置那开始+，+5到达数据长度2，再+数据内容即可
    if (((des_Index + OldProtocol_Package_Info.data_len + 5)) > Rec_Buffer_size) // 判断是否超出缓冲区的范围，超出则需要进行分段CRC计算
    {
        uint16_t offset_temp = Rec_Buffer_size - des_Index;                                                             // 得到超过缓冲区大小的差值
        temp_crc += Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[des_Index], offset_temp);                                 // 先计算未超出部分
        temp_crc += Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[0], OldProtocol_Package_Info.data_len + 5 - offset_temp); // 计算超出的部分
    }
    else
    {
        // 计算CRC(目标地址到数据的最后一个数据)
        temp_crc = Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[des_Index], OldProtocol_Package_Info.data_len + 5);
    }

    if (OldProtocol_Package_Info.crc != temp_crc) // CRC不一致则
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // 解析失败
        return 0x02;
    }
    /* 偏移一个字节习惯(可加可不加反正退出函数后会进行清0) */
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    return 0x00;
}

/**
 * @param    data -> 数据地址
 * @param    len -> 数据长度
 * @retval   CRC结果
 * @brief    旧协议CRC计算
 **/
static uint8_t Bsp_OldProtocol_CRC_Calculate(uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t crc = 0;

    for (i = 0; i < len; i++)
    {
        crc += *(data + i);
    }
    return crc; // 返回的是所有字节的和取低8位
}

/**
 * @param    *deal_param -> 串口协议解析参数结构体指针
 * @param    clear_offset -> 清理包的偏移
 * @retval   None
 * @brief    清除解析过的包，避免被重复解析
 **/
static void Bsp_OldProtocol_ClearParsedPackage(Uart_QueueParse_st *deal_param, uint8_t clear_offset)
{
    // 先判断需要清除的元素是否是队列中最后一个元素,如果不是，则
    if (deal_param->deal_queue_index > (clear_offset + 1))
    {
        // 将它后面的元素向前移动，以覆盖当前元素
        memcpy((void *)&deal_param->deal_queue[clear_offset], (uint8_t *)&deal_param->deal_queue[clear_offset + 1], (deal_param->deal_queue_index - clear_offset - 1) * sizeof(SingleQueueInfo_st));
        deal_param->deal_queue[deal_param->deal_queue_index - 1].count = 0;     // 计数清0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].data_len = 0;  // 数据长度清0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].deal_sign = 0; // 处理接收进度
    }
    else // 如果已经是最后的元素则直接清除对应的队列参数即可
    {
        deal_param->deal_queue[clear_offset].count = 0;
        deal_param->deal_queue[clear_offset].data_len = 0;
        deal_param->deal_queue[clear_offset].deal_sign = 0;
    }
    deal_param->deal_queue_index--; // 索引减1
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★旧协议解析成功处理部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    Rec_Buffer -> 接收缓冲地址
 * @retval   None
 * @brief    旧协议解析成功处理函数
 **/
static void Bsp_OldProtocol_ParsedSuccess_Handler(uint8_t *Rec_Buffer)
{
    Bsp_OldProtocol_SyntheticData_Handler(Rec_Buffer);  // 旧协议处理综合数据
}

/**
 * @param    Rec_Buffer -> 接收缓冲地址
 * @retval   None
 * @brief    旧协议处理综合数据
 **/
static void Bsp_OldProtocol_SyntheticData_Handler(uint8_t *Rec_Buffer)
{
    uint16_t total_len = 0;                                                              // 包的总长度
    total_len = OldProtocol_Package_Info.data_len + OldProtocol_Exclude_Data_PackageLen; // 若包需要转发，需要转发的数据长度

    switch (OldProtocol_Package_Info.des) // 目标地址
    {
    case OLD_OWN_ADDR: // 【当前地址】
    {
        switch (OldProtocol_Package_Info.cmd) // 【功能码】
        {
        case Old_Protocol_CMD_ProtocolSwich: // 【协议切换】
        {
            Bsp_OldProtocol_CmdProtocolSwich_Handler();
            break;
        }
        default:
            break;
        }
        break;
    }
    case OLD_ADDR_KEY_BOARD:
    {
        uint32_t cpu_stat = enter_critical();
        if (OldProtocol_Package_Info.head_position + total_len > RX_BUFFER_SIZE)
        {
            // 若不连续，先发缓冲数组底部数据
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position);
            // 再发缓冲数组顶部数据
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[0], total_len - (RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position));
        }
        else
        {
            // 若连续，直接发
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], OldProtocol_Package_Info.data_len + OldProtocol_Exclude_Data_PackageLen);
        }
        exit_critical(cpu_stat);
        break;
    }
    default:
        break;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★旧协议解析成功功能控制部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    None
* @retval   None
* @brief    【协议切换】命令 处理函数
**/
static void Bsp_OldProtocol_CmdProtocolSwich_Handler(void)
{

    /*发送【确认】信号*/
    uint8_t data[2];
    data[0] = Old_Protocol_CMD_ProtocolSwich;
    data[1] = 0x00;
    LOG_I_Bsp_OldProtocol("src: %x", OldProtocol_Package_Info.src);
    Bsp_OldProtocol_SendPackage(OldProtocol_Package_Info.src, Old_Protocol_CMD_Confirm, 2, data);

    // 串口变量初始化
    Bsp_Uart.Bsp_Uart_ParameterInit();
    System_Status.update_mode = FLAG_true;  // 固件更新模式置位
}