/***************************************************************************
 * File: Bsp_OldProtocol.c
 * Author: Yang
 * Date: 2024-02-18 21:07:14
 * description:
 -----------------------------------
��Э�����
    ��Э��(OldProtoocol)��
    ����ʽ��:
            ֡ͷ--1Byte �̶�ֵ��0x55
            Ŀ�ĵ�ַ--1Byte	��������0x04
            Դ��ַ--1Byte
            ������--1Byte
            �û����ݳ���1--1Byte
            �û����ݳ���2--1Byte ���ݳ���1��2����һ���ģ����Գ���������ֱ���ж����ݳ���2����
            �û�����(UD)--NByte	�û����ݿ���Ϊ��
            У��--1Byte ��У�飺��Ŀ�ĵ�ַ��ʼ���ֽں�
            ��������֡����ȷ�ϡ�55 04 02 00 02 02 53 00 5D
    ���������ȵ��ٸߡ�
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
// ��Э���������Ϣ�ṹ�����
OldProtocol_Package_Info_st OldProtocol_Package_Info = {0};

Bsp_OldProtocol_st Bsp_OldProtocol =
{
    .Bsp_OldProtocol_RxDataParse_Handler = &Bsp_OldProtocol_RxDataParse_Handler,
    .Bsp_OldProtocol_SendPackage = &Bsp_OldProtocol_SendPackage
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ���Э��Ӧ�ò㲿�֡� ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    deal_param -> ����Э����������ṹ��ָ��
 * @param    value_index -> ���ջ��嵱ǰ�������������յ�ǰ����ǰ��
 * @param    Rec_len -> ���յ����ݵĴ�С
 * @retval   None
 * @brief    ��Э��������ݽ�������
 **/
static uint8_t Bsp_OldProtocol_RxDataParse_Handler(Uart_QueueParse_st *deal_param, uint16_t value_index, uint16_t Rec_len)
{
    while (Rec_len != 0)
    {
        for (uint16_t i = 0; i < deal_param->deal_queue_index; i++)
        {
            deal_param->deal_queue[i].deal_sign++; // ����++

            if (OldProtocol_DataLen_Offset == deal_param->deal_queue[i].deal_sign) // ���ȵ����ݳ���ʱ
            {
                deal_param->deal_queue[i].data_len = deal_param->Rec_Buffer_ptr[value_index]; // �����ݳ��ȴ洢��data_len��
            }
            // �� ���� == ���ݳ���+��������İ���С ���ֽ����ʾһ�����������
            else if ((deal_param->deal_queue[i].data_len + OldProtocol_Exclude_Data_PackageLen) == deal_param->deal_queue[i].deal_sign)
            {
                // ��Э���ȡ���ݰ���Ϣ ͨ��Э���ȡ��Ӧλ����Ϣ
                if (!Bsp_OldProtocol_GetPackageInfo(deal_param->Rec_Buffer_ptr, &deal_param->deal_queue[i].count))
                {
                    Bsp_OldProtocol_ParsedSuccess_Handler(deal_param->Rec_Buffer_ptr); // Э������ɹ���������
                }
                Bsp_OldProtocol_ClearParsedPackage(deal_param, i); // ����������İ�
                i--;
            }
        }

        // �����ջ������е������Ƿ��Ǿ�Э��İ�ͷ���������: ��ʾ���յ���һ���µ����ݰ�
        if (OldProtocol_Package_Head == deal_param->Rec_Buffer_ptr[value_index])
        {
            deal_param->deal_queue[deal_param->deal_queue_index].deal_sign++;         // ����ǰ������־����1�����ڼ�¼�����Ľ���
            deal_param->deal_queue[deal_param->deal_queue_index].count = value_index; // �����ջ�����������ֵ������ǰ��������Ԫ�صļ���ֵ���Լ�¼��ͷ�ڽ��ջ������е�λ��
            deal_param->deal_queue_index++;                                           // ��Э�������������1����ʾ��һ���µĴ�������Ԫ�ؼ���
        }
        /*���ݴ�������ݳ��Ȳ���1ʱ��Ҫ++*/
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
 * @param    des->����Ŀ�ĵ�
 * @param    cmd->���Ͱ�����@OldProtocol_CmdType_et
 * @param    datalen->�������ݳ���
 * @param    data->�������ݵ�ַ
 * @retval   None
 * @brief    ��Э�鷢�Ͱ�����
 **/
static void Bsp_OldProtocol_SendPackage(uint8_t des, uint8_t cmd, uint8_t datalen, uint8_t *data)
{
    uint8_t i;
    uint8_t crc;                          // �洢CRCУ���
    uint16_t des_Index;                   // Ŀ���ַ���±�����
    uint8_t Rep_Buffer[Send_Buffer_size]; // ����������ʱ��������
    uint16_t RepBuffer_Index = 0;         // ����������ʱ���������±�����

    Rep_Buffer[RepBuffer_Index] = OldProtocol_Package_Head; // ����ͷ��
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = des; // ��Ŀ���ַ��
    des_Index = RepBuffer_Index;       // �洢Ŀ���ַ���±�����
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = OLD_OWN_ADDR; // ��Դ��ַ(���������Լ���ַ)��
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = cmd; // �������롿
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = datalen; // �����ݳ���1��
    RepBuffer_Index++;
    Rep_Buffer[RepBuffer_Index] = datalen; // �����ݳ���2��
    RepBuffer_Index++;
    // �������
    for (i = 0; i < datalen; i++)
    {
        Rep_Buffer[RepBuffer_Index] = *(data + i);
        RepBuffer_Index++;
    }
    crc = Bsp_OldProtocol_CRC_Calculate(&Rep_Buffer[des_Index], datalen + 5); // ����crc
    Rep_Buffer[RepBuffer_Index] = crc;                                        // ��crcֵ��
    RepBuffer_Index++;

    uint32_t cpu_stat = enter_critical();
    Bsp_Uart.Bsp_Uart_Ble_SendData(Rep_Buffer, RepBuffer_Index);
    exit_critical(cpu_stat);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ���Э���м�㲿�֡� ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    Rec_Buffer -> ���������ַ
 * @param    RecBuffer_count -> ��������������ַ
 * @retval   0x00->�ɹ� 0x01->�����ݴ�С���ڽ��ջ����С 0x02->������crcУ�����
 * @brief    ��Э���ȡ���ݰ���Ϣ ͨ��Э���ȡ��Ӧλ����Ϣ
 **/
static uint8_t Bsp_OldProtocol_GetPackageInfo(uint8_t *Rec_Buffer, uint16_t *RecBuffer_count)
{
    uint8_t temp_crc = 0;   // �洢����CRC�Ľ����ʱ����
    uint16_t des_Index = 0; // �洢Ŀ���ַλ�õ���ʱ����

    OldProtocol_Package_Info.package_state = FLAG_true;          // �����ɹ�
    OldProtocol_Package_Info.head_position = (*RecBuffer_count); // ��¼�洢��ͷ����λ��

    /*��ȡ��Ŀ���ַ--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);     // ��ͷλ�û�����
    OldProtocol_Package_Info.des = Rec_Buffer[(*RecBuffer_count)]; // �洢��Ŀ���ַ
    des_Index = (*RecBuffer_count);                                // ��¼�洢��Ŀ���ַ����Э���е�λ��
    /*��ȡ��Դ��ַ--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.src = Rec_Buffer[(*RecBuffer_count)]; // �洢Դ��ַ
    /*��ȡ������--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.cmd = Rec_Buffer[(*RecBuffer_count)]; // �洢������
    /*��ȡ���ݳ���1--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    OldProtocol_Package_Info.data_len = Rec_Buffer[(*RecBuffer_count)]; // �洢���ݳ���1

    if (OldProtocol_Package_Info.data_len > Rec_Buffer_size)            // ���ݳ��ȳ��������ճ�����
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // ����ʧ��
        return 0x01;
    }

    /*��ȡ���ݳ���2--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    if (OldProtocol_Package_Info.data_len != Rec_Buffer[(*RecBuffer_count)]) // �ж����ݳ���2�Ƿ�������ݳ���1
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // ����ʧ��
        return 0x01;
    }
    /*��ȡ�������һ������λ��--ƫ��1�ֽ�*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    if (OldProtocol_Package_Info.data_len != 0) // �ж����ݳ��Ȳ�Ϊ0
    {
        OldProtocol_Package_Info.first_data_position = (*RecBuffer_count); // �洢������ĵ�һ�����ݵ�λ��
    }
    /*��ȡCRCУ���--ƫ��N�ֽ�(����һ���������һ���ֽ�)*/
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, OldProtocol_Package_Info.data_len); // ƫ�ƶ���->���ݳ��ȸ���С
    OldProtocol_Package_Info.crc = Rec_Buffer[(*RecBuffer_count)];                             // �洢CRCУ���

    // Ŀ���ַλ���ǿ�ʼ+��+5�������ݳ���2����+�������ݼ���
    if (((des_Index + OldProtocol_Package_Info.data_len + 5)) > Rec_Buffer_size) // �ж��Ƿ񳬳��������ķ�Χ����������Ҫ���зֶ�CRC����
    {
        uint16_t offset_temp = Rec_Buffer_size - des_Index;                                                             // �õ�������������С�Ĳ�ֵ
        temp_crc += Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[des_Index], offset_temp);                                 // �ȼ���δ��������
        temp_crc += Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[0], OldProtocol_Package_Info.data_len + 5 - offset_temp); // ���㳬���Ĳ���
    }
    else
    {
        // ����CRC(Ŀ���ַ�����ݵ����һ������)
        temp_crc = Bsp_OldProtocol_CRC_Calculate(&Rec_Buffer[des_Index], OldProtocol_Package_Info.data_len + 5);
    }

    if (OldProtocol_Package_Info.crc != temp_crc) // CRC��һ����
    {
        OldProtocol_Package_Info.package_state = FLAG_false; // ����ʧ��
        return 0x02;
    }
    /* ƫ��һ���ֽ�ϰ��(�ɼӿɲ��ӷ����˳�������������0) */
    Bsp_Uart.Bsp_Uart_RecData_AddPosition(RecBuffer_count, 1);
    return 0x00;
}

/**
 * @param    data -> ���ݵ�ַ
 * @param    len -> ���ݳ���
 * @retval   CRC���
 * @brief    ��Э��CRC����
 **/
static uint8_t Bsp_OldProtocol_CRC_Calculate(uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint8_t crc = 0;

    for (i = 0; i < len; i++)
    {
        crc += *(data + i);
    }
    return crc; // ���ص��������ֽڵĺ�ȡ��8λ
}

/**
 * @param    *deal_param -> ����Э����������ṹ��ָ��
 * @param    clear_offset -> ��������ƫ��
 * @retval   None
 * @brief    ����������İ������ⱻ�ظ�����
 **/
static void Bsp_OldProtocol_ClearParsedPackage(Uart_QueueParse_st *deal_param, uint8_t clear_offset)
{
    // ���ж���Ҫ�����Ԫ���Ƿ��Ƕ��������һ��Ԫ��,������ǣ���
    if (deal_param->deal_queue_index > (clear_offset + 1))
    {
        // ���������Ԫ����ǰ�ƶ����Ը��ǵ�ǰԪ��
        memcpy((void *)&deal_param->deal_queue[clear_offset], (uint8_t *)&deal_param->deal_queue[clear_offset + 1], (deal_param->deal_queue_index - clear_offset - 1) * sizeof(SingleQueueInfo_st));
        deal_param->deal_queue[deal_param->deal_queue_index - 1].count = 0;     // ������0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].data_len = 0;  // ���ݳ�����0
        deal_param->deal_queue[deal_param->deal_queue_index - 1].deal_sign = 0; // �������ս���
    }
    else // ����Ѿ�������Ԫ����ֱ�������Ӧ�Ķ��в�������
    {
        deal_param->deal_queue[clear_offset].count = 0;
        deal_param->deal_queue[clear_offset].data_len = 0;
        deal_param->deal_queue[clear_offset].deal_sign = 0;
    }
    deal_param->deal_queue_index--; // ������1
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ���Э������ɹ��������֡� ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 * @param    Rec_Buffer -> ���ջ����ַ
 * @retval   None
 * @brief    ��Э������ɹ���������
 **/
static void Bsp_OldProtocol_ParsedSuccess_Handler(uint8_t *Rec_Buffer)
{
    Bsp_OldProtocol_SyntheticData_Handler(Rec_Buffer);  // ��Э�鴦���ۺ�����
}

/**
 * @param    Rec_Buffer -> ���ջ����ַ
 * @retval   None
 * @brief    ��Э�鴦���ۺ�����
 **/
static void Bsp_OldProtocol_SyntheticData_Handler(uint8_t *Rec_Buffer)
{
    uint16_t total_len = 0;                                                              // �����ܳ���
    total_len = OldProtocol_Package_Info.data_len + OldProtocol_Exclude_Data_PackageLen; // ������Ҫת������Ҫת�������ݳ���

    switch (OldProtocol_Package_Info.des) // Ŀ���ַ
    {
    case OLD_OWN_ADDR: // ����ǰ��ַ��
    {
        switch (OldProtocol_Package_Info.cmd) // �������롿
        {
        case Old_Protocol_CMD_ProtocolSwich: // ��Э���л���
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
            // �����������ȷ���������ײ�����
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position);
            // �ٷ��������鶥������
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[0], total_len - (RX_BUFFER_SIZE - OldProtocol_Package_Info.head_position));
        }
        else
        {
            // ��������ֱ�ӷ�
            Bsp_Uart.Bsp_Uart_Ble_SendData(&Rec_Buffer[OldProtocol_Package_Info.head_position], OldProtocol_Package_Info.data_len + OldProtocol_Exclude_Data_PackageLen);
        }
        exit_critical(cpu_stat);
        break;
    }
    default:
        break;
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ���Э������ɹ����ܿ��Ʋ��֡� ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    None
* @retval   None
* @brief    ��Э���л������� ��������
**/
static void Bsp_OldProtocol_CmdProtocolSwich_Handler(void)
{

    /*���͡�ȷ�ϡ��ź�*/
    uint8_t data[2];
    data[0] = Old_Protocol_CMD_ProtocolSwich;
    data[1] = 0x00;
    LOG_I_Bsp_OldProtocol("src: %x", OldProtocol_Package_Info.src);
    Bsp_OldProtocol_SendPackage(OldProtocol_Package_Info.src, Old_Protocol_CMD_Confirm, 2, data);

    // ���ڱ�����ʼ��
    Bsp_Uart.Bsp_Uart_ParameterInit();
    System_Status.update_mode = FLAG_true;  // �̼�����ģʽ��λ
}