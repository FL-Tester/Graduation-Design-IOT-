#ifndef GP_MCU2_TASK_QUEUE_H
#define GP_MCU2_TASK_QUEUE_H

#include "main.h"

typedef struct{
    uint8_t     temp;      //温度
    uint8_t     humi;      //湿度
    uint16_t    light;     //光照强度
    uint16_t    soil_humi; //土壤湿度
    uint8_t     water_pump; //水泵挡位
    uint8_t     crc;       //校验
} send_data_t;

//定义一个状态机的消息队列
typedef struct{
    uint8_t type;
    send_data_t data;
} msg_t;




//任务队列
typedef struct Queue{
    msg_t *msg;
    struct Queue *next;
    int size;
} Queue_t;




#endif //GP_MCU2_TASK_QUEUE_H
