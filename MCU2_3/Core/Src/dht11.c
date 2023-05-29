/*
 * dht11.c
 * @discription: DHT11温湿度传感器驱动程序
 * 使用了一个定时4提供的us级延时函数
 * 用的引脚是PA5
 * @version: 1.0
 */

#include "dht11.h"
#include "tim.h"

void DHT11_IO_IN(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void DHT11_IO_OUT(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void DHT11_Rst(void){
    DHT11_IO_OUT(); 	//设置为输出
    //pa5 设置为低电平
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_RESET);
    HAL_Delay(20);    	//至少18ms
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,GPIO_PIN_SET);
    Delay_us(30);     	//主机拉高20~40us
}
uint8_t DHT11_Check(void){
    uint8_t retry=0;
    DHT11_IO_IN();     	//设置为输入
    while (HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5) == GPIO_PIN_SET && retry<100){
        retry++;
        Delay_us(1);
    }
    if( retry >= 100 )
        return 1;
    else
        retry=0;

    while (HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_5)== GPIO_PIN_RESET && retry<100){
        retry++;
        Delay_us(1);
    }
    if(retry>=100)
        return 1;

    return 0;
}

uint8_t DHT11_Read_Bit(void){
    uint8_t retry=0;
    while(GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) && retry<100)
    {
        retry++;
        Delay_us(1);
    }
    retry=0;
    while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) && retry<100)
    {
        retry++;
        Delay_us(1);
    }
    Delay_us(40);

    if(GPIO_PIN_SET==HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5))
        return 1;
    else
        return 0;
}
uint8_t DHT11_Read_Byte(void){
    uint8_t dat = 0;
    for (uint8_t i=0;i<8;i++){
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi){
    uint8_t buf[5];
    DHT11_Rst();
    if(DHT11_Check() == 0)
    {
        for(uint8_t i=0;i<5;i++)
            buf[i] = DHT11_Read_Byte();
        if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
        {
            *humi=buf[0];       //这里省略小数部分
            *temp=buf[2];
        }
    }
    else
        return 1;
    return 0;
}
uint8_t DHT11_Init(void){
    DHT11_Rst();
    return DHT11_Check();
}

