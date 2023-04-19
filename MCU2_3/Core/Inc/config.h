#ifndef GP_MCU2_CONFIG_H
#define GP_MCU2_CONFIG_H

#include "dht11.h"
#include "bh1750.h"
#include "soil_humi.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "dma.h"
#include "main.h"


#define READTIME        30
#define MCU_DEFAULT     0
#define debug           1
#define DHT11_DELAY     (1000 / READTIME + 10)  // > 1s
#define TRAN            0.66
#define BUFFER_SIZE_MAX 100


typedef struct{
    uint8_t     temp;      //温度
    uint8_t     humi;      //湿度
    uint16_t    light;     //光照强度
    uint16_t    soil_humi; //土壤湿度
    uint8_t     water_pump; //水泵挡位
    uint8_t     crc;       //校验
} send_data_t;

enum motor_speed{
    stop = 150,     // 停止
    one = 160,      //档位1
    two = 180,      //档位2
    three = 200,    //档位3
    four = 250      //档位4
};

enum State{
    Idel = 0,
    ReadData,
    ControlServo,
    SendData
};

enum lmit_value{  //温湿度阈值 档位 20 60 70 80 90
    humi_1 = 20,
    humi_2 = 50,
    humi_3 = 70,
    humi_4 = 85,
    humi_5 = 100
};

//定义公共事件
typedef enum{
    EVENT_NULL = 0,
    EVENT_READ_DATA,
    EVENT_CONTROL_SERVO,
    EVENT_SEND_DATA,
    EVENT_LORA_RECV
} event_t;

//定义一个状态机的消息队列
typedef struct{
    event_t type;
    send_data_t data;
} msg_t;


// 控制水泵的参数 因为土壤湿度和湿度代表目前代表湿润 而光照强度和温度到底一定范围后代表适合光合作用
static float soil_humi_weight   = 1.5;
static float temp_weight        = -0.1;
static float humi_weight        = 0.1;
static float light_weight       = -0.5; //输出
static uint8_t temp_min         = 20;
static uint8_t temp_max         = 35;
static uint8_t humi_min         = 40;
static uint8_t humi_max         = 80;
static uint16_t light_min       = 1000;
static uint16_t light_max       = 60000;

uint8_t lora_recv_event_flag = 0;   //lora接收事件标志  优先级
uint8_t lora_recv_flag = 0;         //lora接收标志
char LoRaRxBuffer[100];


#if MCU_DEFAULT
#define LORATIME  4000 / READTIME
//lora地址信道 3个字节 数据头一个字节 温湿度2个字节 光照强度4个字节 土壤湿度2个字节 校验1个字节
uint8_t lora_send_buf[12] ={0x00, 0x0b, 0x17, 0X02, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00 ,0x00};

#else
#define LORATIME  4500 / READTIME
uint8_t lora_send_buf[13] ={0x00, 0x0B, 0x17, 0X03, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00};
#endif

#endif //GP_MCU2_CONFIG_H
