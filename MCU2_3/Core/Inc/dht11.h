// 手册说明 采样周期间隔不得低于1秒钟 ·······
#ifndef MCU2_DHT11_H
#define MCU2_DHT11_H

#include "main.h"

void DHT11_IO_IN(void);
void DHT11_IO_OUT(void);
void DHT11_Rst(void);
uint8_t DHT11_Check(void);
uint8_t DHT11_Read_Bit(void);
uint8_t DHT11_Read_Byte(void);
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);
uint8_t DHT11_Init(void);

#endif //MCU2_DHT11_H
