/***************************************************************************
 * File: Bsp_Bluetooth.c
 * Author: Yang
 * Date: 2024-02-04 15:16:58
 * description:
 -----------------------------------
蓝牙名字：
    默认是格式是: "xxxx"
    当关机时下推摇杆则进入修改名字，在原有基础上增加后缀变成: "xxxx(y)",y为名称计数值每次进入此函数则进行+1,在范围内循环:0~BLE_DEVICE_NAME_MAX_LEN
    修改完需要断电重新启动才会生效，手机上则如果之前有配对过需要重新取消配对才能重新进行连接

 -----------------------------------
****************************************************************************/
#include "AllHead.h"

/* Private function prototypes===============================================*/

static void Bsp_BlueTooth_dev_manager_CallBack(enum dev_evt_type type, union dev_evt_u *evt);
static void Bsp_BlueTooth_gap_manager_CallBack(enum gap_evt_type type, union gap_evt_u *evt, uint8_t con_idx);
static void Bsp_BlueTooth_gatt_manager_CallBack(enum gatt_evt_type type, union gatt_evt_u *evt, uint8_t con_idx);
static void Bsp_BlueTooth_Create_adv_Obj(void);
static void Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(uint8_t att_idx, uint8_t con_idx);
static void Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(uint8_t att_idx, uint8_t con_idx, uint16_t length, uint8_t const *value);
static void Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(uint8_t con_idx);

/* Public function prototypes=========================================================*/

static void Bsp_BlueTooth_Init(void);
static void Bsp_BlueTooth_Start_adv(void);
static void Bsp_BlueTooth_Send_Notification(void);

/* Private variables=========================================================*/
// 蓝牙服务 UUID标识
static const uint8_t ble_svc_uuid_128[] =
{
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
    0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e
};
// 蓝牙串口接收服务 UUID标识
static const uint8_t ble_rx_char_uuid_128[] =
{
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
    0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e
};
// 蓝牙串口发送服务 UUID标识
static const uint8_t ble_tx_char_uuid_128[] =
{
    0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
    0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e
};

static const uint8_t att_decl_char_array[] = {0x03, 0x28};  // 定义 ATT 属性声明 UUID
static const uint8_t att_desc_client_char_cfg_array[] = {0x02, 0x29};   // 定义 ATT 描述符客户端特征配置 UUID

static struct gatt_svc_env ls_ble_server_svc_env;   // GATT服务 -- 蓝牙服务句柄

// 属性声明(复制例程即可)
static const struct att_decl ls_ble_server_att_decl[BLE_SVC_ATT_NUM] =
{
    // 【UART 接收特征句柄】
    [BLE_SVC_IDX_RX_CHAR] =
    {
        .uuid = att_decl_char_array,  // 属性声明 UUID
        .s.max_len = 0,               // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT, // UUID 长度
        .s.read_indication = 1,       // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,         // 读请求启用
    },
    // 【UART 接收值句柄】
    [BLE_SVC_IDX_RX_VAL] =
    {
        .uuid = ble_rx_char_uuid_128,    // 属性声明 UUID
        .s.max_len = BLE_SVC_RX_MAX_LEN, // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_128BIT,   // UUID 长度
        .s.read_indication = 1,          // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.wr_cmd = 1,           // 写入允许
        .char_prop.wr_req = 1,           // 写请求启用
    },
    // 【UART 发送特征句柄】
    [BLE_SVC_IDX_TX_CHAR] =
    {
        .uuid = att_decl_char_array,  // 属性声明 UUID
        .s.max_len = 0,               // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT, // UUID 长度
        .s.read_indication = 1,       // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,         // 读请求启用
    },
    // 【UART 发送值句柄】
    [BLE_SVC_IDX_TX_VAL] =
    {
        .uuid = ble_tx_char_uuid_128,    // 属性声明 UUID
        .s.max_len = BLE_SVC_TX_MAX_LEN, // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_128BIT,   // UUID 长度
        .s.read_indication = 1,          // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.ntf_en = 1,           // 启用通知
    },
    // 【UART 发送通知配置句柄】
    [BLE_SVC_IDX_TX_NTF_CFG] =
    {
        .uuid = att_desc_client_char_cfg_array, // 属性声明 UUID
        .s.max_len = 0,                         // 属性支持的最大长度，以字节为单位
        .s.uuid_len = UUID_LEN_16BIT,           // UUID 长度
        .s.read_indication = 1,                 // 触发读取指示  1:表示读取请求将被转发到应用程序
        .char_prop.rd_en = 1,                   // 读请求启用
        .char_prop.wr_req = 1,                  // 写请求启用
    }
};

// 属性声明完成后进行服务声明(复制例程即可)
static const struct svc_decl ls_ble_server_svc =
{
    .uuid = ble_svc_uuid_128,                           // 服务的 UUID
    .att = (struct att_decl *)ls_ble_server_att_decl,   // 服务中包含的属性(结构体指针)
    .nb_att = BLE_SVC_ATT_NUM,                          // 服务中包含的属性数量
    .uuid_len = UUID_LEN_128BIT,                        // 服务的 UUID 的长度
};

/*广播相关*/
static uint8_t ble_scan_rsp_data[31];   // 扫描回复数据包数组(这里不需要回复！)
char ble_adv_name[sizeof(BLE_NAME) + 3] = BLE_NAME; // 蓝牙广播名字存储数组(+3是为了后面修改名字时加后缀)
uint8_t ble_mac_addr[6] = {0};  // 蓝牙MAC地址
uint8_t ble_adv_data[28] = {0};    // 广播数据数组


/* Public variables==========================================================*/
BLE_AdvInfo_st BLE_AdvInfo =
{
    .ble_name_count = 0,
    .ble_adv_name_ptr = ble_adv_name,
    .ble_mac_addr_ptr = ble_mac_addr,
    .ble_adv_handle = 0,
    .ble_adv_data_ptr = ble_adv_data
};

BLE_UartInfo_st BLE_UartInfo =
{
    .reply_data = 0,
    .ble_server_mtu = BLE_SERVER_MTU_DFT
};

Bsp_BlueTooth_st Bsp_BlueTooth =
{
    .ble_adv_info_Instance = &BLE_AdvInfo,
    .ble_uart_info_Instance = &BLE_UartInfo,

    .ble_connect_id = BLE_DISCONNECTED_ID,

    .Bsp_BlueTooth_Init = &Bsp_BlueTooth_Init,
    .Bsp_BlueTooth_Start_adv = &Bsp_BlueTooth_Start_adv,
    .Bsp_BlueTooth_Send_Notification = &Bsp_BlueTooth_Send_Notification
};


/**
* @param    None
* @retval   None
* @brief    蓝牙协议栈/服务初始化
**/
static void Bsp_BlueTooth_Init(void)
{
    dev_manager_init(Bsp_BlueTooth_dev_manager_CallBack);   // 初始化 设备管理器回调函数
    gap_manager_init(Bsp_BlueTooth_gap_manager_CallBack);   // 初始化 GAP回调函数
    gatt_manager_init(Bsp_BlueTooth_gatt_manager_CallBack); // 初始化 GATT回调函数
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙回调函数部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    type -> 设备管理器中的事件类型
* @param    *evt -> 设备事件联合体
* @retval   None
* @brief    处理设备管理器的事件回调(包括初始化 BLE 栈、添加服务、注册 GATT 服务、创建广播对象、启动广播等)
**/
static void Bsp_BlueTooth_dev_manager_CallBack(enum dev_evt_type type, union dev_evt_u *evt)
{
    LOG_I_Bsp_BlueTooth("[Device evt:%d]", type);

    switch (type)
    {
    case STACK_INIT: // 【栈初始化事件】
    {
        struct ble_stack_cfg cfg =
        {
            .private_addr = false,
            .controller_privacy = false,
        };
        dev_manager_stack_init(&cfg);
        break;
    }
    case STACK_READY: // 【栈就绪事件】
    {
        /*获取MAC地址*/
        bool type;
        dev_manager_get_identity_bdaddr(Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr, &type);  // 初始化蓝牙地址
        LOG_I_Bsp_BlueTooth("type: %d, addr:", type);                                                       // 【调试】
        LOG_HEX_Bsp_BlueTooth(Bsp_BlueTooth.ble_adv_info_Instance->ble_mac_addr_ptr, sizeof(ble_mac_addr)); // 【调试】

        /*添加服务*/
        dev_manager_add_service((struct svc_decl *)&ls_ble_server_svc);  // 添加蓝牙服务

        /*其他用户代码*/
        Bsp_SysTimer.Bsp_SysTimer_Init();             // 系统软件定时器初始化
        Bsp_Key.Bsp_key_Timer_Init();                 // 按键软件定时器初始化
        break;
    }
    case SERVICE_ADDED: // 【添加服务事件】
    {
        /*在GATT管理器中注册服务*/
        // 参数1: 服务的启动句柄 参数2: 服务属性数量 参数3：要注册的服务指针
        gatt_manager_svc_register(evt->service_added.start_hdl, BLE_SVC_ATT_NUM, &ls_ble_server_svc_env);
        Bsp_BlueTooth_Create_adv_Obj(); // 创建广播状态
        break;
    }
    case ADV_OBJ_CREATED: // 【创建广播对象事件】
    {
        LS_ASSERT(0 == evt->obj_created.status);                                       // 值不为 0，则会触发断言失败
        Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle = evt->obj_created.handle; // 获取句柄
        if (FLAG_true == System_Status.sys_power_switch)                               // 开机即开始广播
        {
            Bsp_BlueTooth_Start_adv();
        }
        break;
    }
    case ADV_STOPPED: // 【广播停止事件】
    {
        LOG_I_Bsp_BlueTooth("adv stop!");
        break;
    }
    default:
        break;
    }
}

/**
* @param    type -> GAP中的事件类型
* @param    *evt -> GAP联合体
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    处理 GAP 回调函数(设备如何发现、连接，以及为用户提供有用的信息)
**/
static void Bsp_BlueTooth_gap_manager_CallBack(enum gap_evt_type type, union gap_evt_u *evt, uint8_t con_idx)
{
    LOG_I_Bsp_BlueTooth("[gap ev: %d]", type);    // 【调试】

    switch (type)
    {
    case CONNECTED: // 【连接事件】
    {
        Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, ble_connect_blink, BLE_CONNECT_BLINK_TIME);
        System_Status.bluetooth = FLAG_true;    // 蓝牙连接信号置1
        Bsp_BlueTooth.ble_connect_id = con_idx; // 存储连接ID
        LOG_I_Bsp_BlueTooth(".......connected!.......");    // 【调试】
        break;
    }
    case DISCONNECTED: // 【断开连接事件】
    {
        System_Status.bluetooth = FLAG_false;    // 蓝牙连接信号置0
        Bsp_BlueTooth.ble_connect_id = BLE_DISCONNECTED_ID;    // 未连接状态
        // 如果开机且不处于低电量模式 则重新打开广播
        if(FLAG_true == System_Status.sys_power_switch && System_Status.low_power_mode != FLAG_true)
        {
            Bsp_Led.Bsp_Led_BlinkConrol_Handler(LED_1, blink, BLE_DISCONNECT_BLINK_TIME);
            Bsp_BlueTooth.Bsp_BlueTooth_Start_adv();
        }
        LOG_I_Bsp_BlueTooth(".......disconnected!.......");    // 【调试】
        break;
    }
    case CONN_PARAM_REQ: // 【连接参数请求事件】
    {
        break;
    }
    case CONN_PARAM_UPDATED: // 【连接参数更新事件】
    {
        break;
    }
    case MASTER_PAIR_REQ: // 【主机配对请求事件】
    {
        break;
    }
    default:
        break;
    }
}

/**
* @param    type -> GATT中的事件类型
* @param    *evt -> GATT联合体
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    处理 GATT 回调函数(为蓝牙设备间的通信提供了标准的规范和约束)
**/
static void Bsp_BlueTooth_gatt_manager_CallBack(enum gatt_evt_type type, union gatt_evt_u *evt, uint8_t con_idx)
{
    LOG_I_Bsp_BlueTooth("[gatt ev: %d]", type); // 【调试】

    switch (type)
    {
    case SERVER_READ_REQ: // 【APP读请求】
    {
        LOG_I_Bsp_BlueTooth("gatt read req");   // 【调试】
        Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(evt->server_read_req.att_idx, con_idx);
        break;
    }
    case SERVER_WRITE_REQ: // 【APP写请求(在这里进行接收数据)】
    {
        LOG_I_Bsp_BlueTooth("gatt write req");   // 【调试】
        // 参数1： 属性索引 参数2： 连接ID 参数3： 收到的数据长度 参数4： 收到的数据指针
        Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(evt->server_write_req.att_idx, con_idx, evt->server_write_req.length, evt->server_write_req.value);
        break;
    }
    case SERVER_NOTIFICATION_DONE: // 【服务器发送通知完成(即APP发送数据完成)】
    {
        LOG_I_Bsp_BlueTooth("gatt ntf done");   // 【调试】
        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag = FLAG_true;   // 设置状态为发送不忙
        break;
    }
    case MTU_CHANGED_INDICATION: // 【MTU交换指示，适用于客户端和服务器】
    {
        Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu = evt->mtu_changed_ind.mtu;
        LOG_I_Bsp_BlueTooth("gatt mtu: %d", Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu);   // 【调试】
        Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(con_idx); // 更新指定连接的数据包长度
        break;
    }
    default:
    {
        LOG_I_Bsp_BlueTooth("gatt not handled!");   // 【调试】
        break;
    }
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--dev部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--GAP相关★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--GATT相关★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    att_idx -> 属性索引
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    蓝牙服务器(APP)读请求处理(即在这里进行回复APP内容)
**/
static void Bsp_BlueTooth_Gatt_ServerReadQequest_Handler(uint8_t att_idx, uint8_t con_idx)
{
    uint16_t handle = 0; // 初始化属性句柄为0

    // 发送通知配置
    if(att_idx == BLE_SVC_IDX_TX_NTF_CFG)
    {
        // 获取BLE服务的属性句柄
        handle = gatt_manager_get_svc_att_handle(&ls_ble_server_svc_env, att_idx);
        // 回复读取请求 数据长度为2字节(uint16_t类型)
        gatt_manager_server_read_req_reply(con_idx, handle, 0, (void *)&Bsp_BlueTooth.ble_uart_info_Instance->reply_data, 2);
    }
    LOG_I_Bsp_BlueTooth("gatt att_idx: %d", att_idx);    // 【调试】属性索引
}

/**
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @retval   None
* @brief    更新指定连接的数据包长度
**/
static void Bsp_BlueTooth_Gatt_ServerDataPackageLenUpdate(uint8_t con_idx)
{
    // 定义一个结构体变量 dlu_param，用于设置数据包大小
    struct gap_set_pkt_size dlu_param =
    {
        .pkt_size = 251, // 设置数据包大小为251字节
    };
    // 调用 gap_manager_set_pkt_size 函数设置连接索引为 con_idx 的数据包大小为 251 字节
    gap_manager_set_pkt_size(con_idx, &dlu_param);
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--广播部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/**
* @param    None
* @retval   None
* @brief    开始广播
**/
static void Bsp_BlueTooth_Start_adv(void)
{
    LS_ASSERT(Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle != 0xff); // 断言不等于0xff，如果等于则会触发断言错误

    /* 发送广播数据 */
    uint8_t adv_data_length = ADV_DATA_PACK(
                                  Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_data_ptr,         // 广播数据存储的数组
                                  1,                                                             // 广播数据项的数量
                                  GAP_ADV_TYPE_SHORTENED_NAME,                                   // 广播类型---缩短的蓝牙名称
                                  BLE_NAME,         // 广播数据---蓝牙名称存储数组
                                  sizeof(BLE_NAME)                                          // 广播数据的长度(此处不能用指针)
                              );
    // 开始广播  参数1：广播句柄 参数2：广播数据 参数3：广播数据长度 参数4：响应数据包 参数5：响应数据包长度(如果没有advertising_data或scan_response_data，对应的length需要填0。不可以填如与实际内容不匹配的length)
    dev_manager_start_adv(Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_handle, Bsp_BlueTooth.ble_adv_info_Instance->ble_adv_data_ptr, adv_data_length, ble_scan_rsp_data, 0);
    LOG_I_Bsp_BlueTooth("adv start");
}

/**
* @param    None
* @retval   None
* @brief    创建广播对象
**/
static void Bsp_BlueTooth_Create_adv_Obj(void)
{
    // 定义结构体 legacy_adv_obj_param 的实例 adv_param，并初始化成员变量
    struct legacy_adv_obj_param adv_param =
    {
        // 广播间隔的最小值(一般跟max配置成同一个值)，单位为 0.625ms（0.625 * 32 = 20ms）
        .adv_intv_min = 0x20,
        // 广播间隔的最大值，单位为 0.625ms
        .adv_intv_max = 0x20,
        // 广播发送设备的地址类型(蓝牙MAC地址)---可以是公有地址或随机静态地址
        .own_addr_type = PUBLIC_OR_RANDOM_STATIC_ADDR,
        // 广播过滤策略，此处为不进行过滤
        .filter_policy = 0,
        // 广播信道映射，指定广播数据发送的信道---Bit0:启用37通道 Bit1:启用38通道 Bit2:启用39通道(7表示3个通道上都发送)
        .ch_map = 0x7,
        // 广播模式，此处为通用可发现模式，即不针对特定设备进行广告
        .disc_mode = ADV_MODE_GEN_DISC,
        // 广播属性，包括是否可连接、是否可扫描、是否定向广告、是否高占空比
        .prop = {
            // 可连接属性，是否被连接
            .connectable = 1,
            // 可扫描属性，是否可被其他设备扫描到
            .scannable = 1,
            // 定向属性，不定向
            .directed = 0,
            // 高占空比属性
            .high_duty_cycle = 0,
        },
    };
    // 创建广播对象
    dev_manager_create_legacy_adv_object(&adv_param);
}

/**
* @param    att_idx -> 属性索引
* @param    con_idx -> 连接ID，用于标识当前连接的设备
* @param    length -> 收到的数据长度
* @param    *value -> 收到的数据指针
* @retval   None
* @brief    蓝牙服务器(APP)写请求处理(即在这里进行接收APP发送过来的内容)
**/
static void Bsp_BlueTooth_Gatt_ServerWriteQequest_Handler(uint8_t att_idx, uint8_t con_idx, uint16_t length, uint8_t const *value)
{
    if (att_idx == BLE_SVC_IDX_RX_VAL)  // 是接收属性则
    {
        if (FLAG_false == Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag)   // 接收忙
        {
            LOG_I_Bsp_BlueTooth("tx busy, data discard!");
        }
        else
        {
            Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag = FLAG_false;    // 设置状态为接收忙

            // 如果蓝牙接收数据大于蓝牙接收缓冲,则需要分批存储
            if ((Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index + length) > BLE_SVC_BUFFER_SIZE)
            {
                // 存入底部(即缓冲区剩余可存储的部分)
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index], (uint8_t *)value, BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index);
                // 覆盖顶部
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[0], (uint8_t *)(value + BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index), length - (BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index));

                // 开机 或者 电机校准模式下
                if (FLAG_true == System_Status.sys_power_switch)
                {
                    if (FLAG_true == System_Status.update_mode) // 升级模式
                    {
                        Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                    else
                    {
                        Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                }
                Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index = length - (BLE_SVC_BUFFER_SIZE - Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index);
            }
            else
            {
                // 直接写入缓存
                memcpy(&Bsp_Uart.UartInnfo[UART_BLE].uart_rx_buffer_ptr[Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index], (uint8_t *)value, length);

                if (FLAG_true == System_Status.sys_power_switch)
                {
                    if (FLAG_true == System_Status.update_mode) // 升级模式
                    {
                        Bsp_NewProtocol.Bsp_NewProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                    else
                    {
                        Bsp_OldProtocol.Bsp_OldProtocol_RxDataParse_Handler(&Uart_QueueParse_Ble, Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index, length);
                    }
                }
                Bsp_Uart.UartInnfo[UART_BLE].uart_rx_index += length;
            }

            Bsp_Uart.UartInnfo[UART_BLE].uart_rx_busy_Flag = FLAG_true;    // 设置状态为接收空闲
        }
    }
    else if (att_idx == BLE_SVC_IDX_TX_NTF_CFG) // 发送通知配置
    {
        LS_ASSERT(2 == length);
        memcpy(&Bsp_BlueTooth.ble_uart_info_Instance->reply_data, value, length);
    }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ★蓝牙服务--串口部分★ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
* @param    None
* @retval   None
* @brief    蓝牙发送通知(数据)给客户端
**/
static void Bsp_BlueTooth_Send_Notification(void)
{
    // 串口发送缓存有数据且发送缓冲不忙
    if ((Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index > 0) && (FLAG_true == Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag))
    {
        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_busy_Flag = FLAG_false;    // 正在进行服务器通知(忙状态)

        uint16_t handle = gatt_manager_get_svc_att_handle(&ls_ble_server_svc_env, BLE_SVC_IDX_TX_VAL);  // 获取发送数据服务句柄
        // 判断是否超过最大发送长度  co_min->返回两个数最小一个
        uint16_t tx_len = Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index > co_min(BLE_SERVER_MAX_DATA_LEN, BLE_SVC_TX_MAX_LEN) ? co_min(BLE_SERVER_MAX_DATA_LEN, BLE_SVC_TX_MAX_LEN) : Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index;

        Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index -= tx_len;
        // 发送数据到客户端（UART）
        gatt_manager_server_send_notification(Bsp_BlueTooth.ble_connect_id, handle, Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr, tx_len, NULL);
        // 将没发送的数据向左偏移
        memcpy((void *)&Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr[0], (void *)&Bsp_Uart.UartInnfo[UART_BLE].uart_tx_buffer_ptr[tx_len], Bsp_Uart.UartInnfo[UART_BLE].uart_tx_index);
    }
}