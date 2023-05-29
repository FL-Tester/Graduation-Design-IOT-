#include "stdio.h"
#include "config.h"
#include "string.h"

volatile event_t event_flag = EVENT_NULL;       //事件标志
send_data_t sensor_data;
enum State CurrentState = Idel;                 // 当前状态
enum motor_speed motor_speed = stop;            // 水泵默认档位
uint8_t lora_receive_buffer[BUFFER_SIZE_MAX];   //lora接收缓冲区
uint8_t lora_recv_flag = 0;                     //lora接收标志
uint8_t lora_recv_len = 0;                      //lora接收长度
uint8_t auto_mode = 1;
int main(void){
    hw_init();  //硬件初始化
    while (1){
        switch (CurrentState) {
            case ReadData:
                read_sensor_data_task();  // 读取传感器数据
                break;
            case ControlServo:
                control_servo_task();     // 控制水泵
                break;
            case SendData:
                send_sensor_data_task();  // 发送传感器数据
                break;
            case RecvControl:
                recv_control_task();      // 接收控制命令
                break;
            default:
                idel_task();              // 空闲任务
                break;
        }


    }
}
//定时器 驱动数据读取和发送
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    static unsigned char ledState = 0, count = 0;
    if (htim == (&htim4)){
        event_flag = EVENT_READ_DATA;
        count++;
    }
    if (count == 3){
        event_flag = EVENT_SEND_DATA;
        count = 0;
    }
}

//空闲任务
void idel_task(void){
    while ( CurrentState == Idel){
        switch (event_flag) {
            case EVENT_READ_DATA:
                event_flag = EVENT_NULL;
                CurrentState = ReadData;
                break;
            case EVENT_SEND_DATA:
                event_flag = EVENT_NULL;
                CurrentState = SendData;
                break;
            case EVENT_LORA_RECV:
                event_flag = EVENT_NULL;
                CurrentState = RecvControl;
                break;
            default:
                break;
        }
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}
//1.空气中湿度为：0
////2.水中湿度为：100
////3. 土壤中从浅到深 44 ~ 53%
//读取数据任务
void read_sensor_data_task(void){
    static int Dht11ReadStatus = -1;
    if (Dht11ReadStatus == DHT11_DELAY || Dht11ReadStatus == -1){    //采样周期 > 1s
        Dht11ReadStatus = 0;
        Dht11ReadStatus = DHT11_Read_Data(&sensor_data.temp, &sensor_data.humi); //读取温湿度
    }else
        Dht11ReadStatus++;
    sensor_data.light = bh1750_read();
    sensor_data.soil_humi = soil_humi_read();  // 0 ~ 5V -> 0 ~ 100%
#if debug
    printf("1.数据读取任务:温度:%d,湿度:%d,光照强度:%d,土壤湿度:%d\n",
           sensor_data.temp, sensor_data.humi, sensor_data.light,
           sensor_data.soil_humi);//log
#endif
    CurrentState = EVENT_CONTROL_SERVO;
    CurrentState = ControlServo; //状态切换
}
//控制舵机任务  //需要继续优化 因为没有参考温湿度和光照强度的值
void control_servo_task(void){
    if (auto_mode == 1)
        goto controlend;
    // 归一化
    float soil_humi_factor = sensor_data.soil_humi / 100.0f; //土壤湿度
    float clamped_light = sensor_data.light; //关照强度
    if (clamped_light < light_min) {      //关照强度作用范围
        clamped_light = light_min;
    } else if (clamped_light > light_max) {
        clamped_light = light_max;
    }
    float  light_factor = (clamped_light - light_min) / (light_max - light_min);
    float clamped_temp = sensor_data.temp;   //温湿度
    float clamped_humi = sensor_data.humi;
    if (clamped_temp < temp_min) {
        clamped_temp = temp_min;
    } else if (clamped_temp > temp_max) {
        clamped_temp = temp_max;
    }
    if (clamped_humi < humi_min) {
        clamped_humi = humi_min;
    } else if (clamped_humi > humi_max) {
        clamped_humi = humi_max;
    }
    float  temp_factor = (clamped_temp - temp_min) / (temp_max - temp_min);
    float  humi_factor = (clamped_humi - humi_min) / (humi_max - humi_min);
    float combined_factor = soil_humi_weight * soil_humi_factor +
                            temp_weight * temp_factor +
                            humi_weight * humi_factor +
                            light_weight * light_factor;

    // 根据 combined_factor 计算水泵输出挡位
    if (combined_factor < 0.2) {
        sensor_data.water_pump = four;
    } else if (combined_factor < 0.25) {
        sensor_data.water_pump = three;
    } else if (combined_factor < 0.4) {
        sensor_data.water_pump = two;
    } else if (combined_factor < 0.5) {
        sensor_data.water_pump = one;
    } else {
        sensor_data.water_pump = stop;
    }
    //舵机模拟水流大小
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sensor_data.water_pump);

#if debug
    printf("2.控制舵机任务:pidout = %f\n", sensor_data.water_pump); //log
    printf("soil_humi_factor:%f,temp_factor:%f,humi_factor:%f,light_factor:%f,combined_factor:%f\n",
           soil_humi_factor, temp_factor, humi_factor, light_factor, combined_factor);
#endif


controlend:
    event_flag = EVENT_NULL;
    CurrentState = Idel;
}
//发送数据任务
void send_sensor_data_task(void){
//数据打包
    lora_send_buf[4] = sensor_data.temp;   //温湿度
    lora_send_buf[5] = sensor_data.humi;
    lora_send_buf[6] = sensor_data.soil_humi >> 8; //土壤湿度 2个字节 拆分出来
    lora_send_buf[7] = sensor_data.soil_humi & 0xff;
    lora_send_buf[8] = sensor_data.light >> 8;
    lora_send_buf[9] = sensor_data.light & 0xff;
    if (sensor_data.water_pump == stop)
        lora_send_buf[10] = 0;
    else if (sensor_data.water_pump == one)
        lora_send_buf[10] = 1;
    else if (sensor_data.water_pump == two)
        lora_send_buf[10] = 2;
    else if (sensor_data.water_pump == three)
        lora_send_buf[10] = 3;
    else if (sensor_data.water_pump == four)
        lora_send_buf[10] = 4;
    lora_send_buf[11] = lora_send_buf[4] + lora_send_buf[5] + lora_send_buf[6] +
                        lora_send_buf[7] + lora_send_buf[8] +
                        lora_send_buf[9] + lora_send_buf[10]; //校验
//发送数据
    HAL_UART_Transmit(&huart2, lora_send_buf, 12, 1000);//发送数据
    printf("3.LORA发送任务:发送成功!\n"); //log
    event_flag = EVENT_NULL;
    CurrentState = Idel; //状态切换
}

//lora接收任务
void recv_control_task(void){
    uint8_t control_byte = lora_receive_buffer[0];
    printf("4.LORA接收任务:开始接收!\n"); //log
    printf ("%x\n", lora_receive_buffer[0]);
    uint8_t node = (control_byte & 0xC0) >> 6;
    uint8_t mode = (control_byte & 0x30) >> 4;
    uint8_t switch_status = control_byte & 0x0F;
    //打印node mode switch_status
    printf("node:%d,mode:%d,switch_status:%d\n", node, mode, switch_status);

    if (node == MCU_NUM){
        if (mode == 1) {
            auto_mode = 1;
            if (switch_status == 1) {
                sensor_data.water_pump = four;
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sensor_data.water_pump);
            } else if (switch_status == 0) {
                sensor_data.water_pump = stop;
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, sensor_data.water_pump);
            }
        }else if (mode == 0)
            auto_mode = 0;
    }
    //清除
    lora_recv_flag = 0;
    lora_recv_len = 0;
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    memset(lora_receive_buffer, 0, BUFFER_SIZE_MAX);
    HAL_UART_Receive_DMA(&huart2, lora_receive_buffer, BUFFER_SIZE_MAX);
    event_flag = EVENT_NULL;
    CurrentState = Idel; //状态切换
}

void hw_init(void){
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();           //led dht11
    MX_DMA_Init();          //DMA初始化  tnnd 找半天 就说为什么可以触发中断 但是不可以收到准确信息 原来是生成后 复制原来的 没有复制DMA
    MX_ADC1_Init();           //土壤湿度
    MX_I2C1_Init();           //bh1750
    MX_USART1_UART_Init();    //log
    MX_USART2_UART_Init();    //lora
    MX_TIM3_Init();           //us延时
    MX_TIM2_Init();           //舵机
    MX_TIM4_Init();           //定时器
    bh1750_init();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim4);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);
    HAL_UART_Receive_DMA(&huart2, lora_receive_buffer, BUFFER_SIZE_MAX);

}

void SystemClock_Config(void){ //
    ;
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void){
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();   //关中断
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */



/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */