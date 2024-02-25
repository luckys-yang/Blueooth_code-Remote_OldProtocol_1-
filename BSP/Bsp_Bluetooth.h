#ifndef __BSP_BLUETOOTH_H
#define __BSP_BLUETOOTH_H

// 信息打印
#define LOG_I_Bsp_BlueTooth(...) LOG_I(__VA_ARGS__)
#define LOG_HEX_Bsp_BlueTooth(data_pointer, data_length) LOG_HEX(data_pointer, data_length)

/*--------------------------- 通用UUID 宏定义 ---------------------------*/
#define GATT_UUID_HID 0x1812 // HID人机接口设备

/*----------------------------蓝牙数据相关 宏定义 --------------------------*/
#define BLE_SERVER_MAX_MTU 247                       // 蓝牙数据包最大长度(MTU)
#define BLE_SERVER_MTU_DFT 23                        // 蓝牙数据包默认长度(MTU)
#define BLE_SERVER_MAX_DATA_LEN (Bsp_BlueTooth.ble_uart_info_Instance->ble_server_mtu - 3) // 蓝牙数据包中数据最大长度
#define BLE_SVC_RX_MAX_LEN (BLE_SERVER_MAX_MTU - 3)  // 蓝牙接收的数据包中数据最大长度
#define BLE_SVC_TX_MAX_LEN (BLE_SERVER_MAX_MTU - 3)  // 蓝牙发送数据包中数据最大长度

/*----------------------------蓝牙设备信息相关(文件系统) 宏定义 --------------------------*/


/*----------------------------蓝牙配对秘钥相关(SEC) 宏定义 --------------------------*/

// 限制蓝牙名称后缀计数的最大范围(0~9)
#define BLE_DEVICE_NAME_MAX_LEN 10

// 蓝牙名称(包含最后的\0)，固定格式不改否则配套APP会检测不到
#define BLE_NAME "520caimei"

// 蓝牙没有连接时ID
#define BLE_DISCONNECTED_ID 0xFF

/*定义 UART 服务的属性索引枚举*/
typedef enum
{
    BLE_SVC_IDX_RX_CHAR,    // UART 接收特征句柄
    BLE_SVC_IDX_RX_VAL,     // UART 接收值句柄
    BLE_SVC_IDX_TX_CHAR,    // UART 发送特征句柄
    BLE_SVC_IDX_TX_VAL,     // UART 发送值句柄
    BLE_SVC_IDX_TX_NTF_CFG, // UART 发送通知配置句柄
    BLE_SVC_ATT_NUM         // UART 服务属性数量
} ble_svc_att_db_handles_et;

/*蓝牙广播信息结构体*/
typedef struct
{
    uint8_t ble_name_count;    // 蓝牙名称计数
    char *ble_adv_name_ptr;    // 存储蓝牙广播名字存储数组指针(+3是为了后面修改名字时加后缀)
    uint8_t *ble_mac_addr_ptr; // 存储蓝牙MAC地址数组指针
    uint8_t ble_adv_handle;    // 广播句柄
    uint8_t *ble_adv_data_ptr; // 广播数据数组指针
} BLE_AdvInfo_st;

/*蓝牙串口信息结构体*/
typedef struct
{
    uint16_t reply_data;    // 服务器回复数据
    uint16_t ble_server_mtu;    // 蓝牙发送服务包大小
} BLE_UartInfo_st;

typedef struct
{
    BLE_AdvInfo_st *ble_adv_info_Instance; // 创建蓝牙广播信息结构体变量指针
    BLE_UartInfo_st *ble_uart_info_Instance;    // 创建蓝牙串口信息结构体变量指针

    uint8_t ble_connect_id;                // 设备连接ID 未连接时为0XFF
    void (*Bsp_BlueTooth_Init)(void);      // 蓝牙协议栈/服务初始化
    void (*Bsp_BlueTooth_Start_adv)(void); // 蓝牙广播开始
    void (*Bsp_BlueTooth_Send_Notification)(void); // 蓝牙发送通知(数据)给客户端
} Bsp_BlueTooth_st;

extern Bsp_BlueTooth_st Bsp_BlueTooth;
#endif
