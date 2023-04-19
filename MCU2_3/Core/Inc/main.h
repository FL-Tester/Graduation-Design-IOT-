#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void SystemClock_Config(void);
void idel_task(void);
void read_sensor_data_task(void);
void control_servo_task(void);
void send_sensor_data_task(void);
void hw_init(void);
void Error_Handler(void);

#define DHT11_PIN_Pin GPIO_PIN_5
#define DHT11_PIN_GPIO_Port GPIOA

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */