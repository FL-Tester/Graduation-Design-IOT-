# Graduation-Design-IOT-

## 基于物联网的校园林木灌溉系统
### 简介
大概就是物联网那一套
1. 下位机 两个stm32 一个网关esp32 之间通信采用lora
2. 腾讯云服务器采用 自己搭建云平台 主要是安装了emqx和mysql
3. 上位机采用pyqt5 

看图 不多bb
![下位机](/RES/ee.png)

### 实现
#### MCU1 (vscode + idf)
采用esp32作为网关，通过lora与mcu2/3通信，通过wifi与服务器通信传输协议是mqtt
代码实现是安装idf串口例子修改的 懒得修改了

1.串口和一个收到的消息发送的消息队列里 所以需要一个任务等待消息队列 读取数据 需要注意的 lora串口发出来的数据 并不是连续的 所以需要读取想要大小的数据 保证数的准确性和完整性

2.当数据接收完毕后 使用freertos的通知机制 通知另一个任务去处理数据 该任务主要是将数据打包成为json格式 发布到mqtt服务器
#### MCU2/3 (clion + stm32cubemx)
采用stm32作为监控节点，通过lora与网关(mcu1)通信
值得注意的是采集的数据有 温湿度 光照强度 土壤湿度 控制的是水泵的开关

1. 写了一个很简单状态机.... 适合的外设也简单 i2c的bh1750 串口的lora和温湿度传感器 以及一个pwm控制的水泵 实现不同速度模拟挡位
#### 打板和pcb
就一个底座 esp32难得打了 就不贴了

#### 服务器搭建和配置
这个更简单了 买一个服务器 主要有公网IP 
去emqx官网文档 查找相关下载指令
安装mysql的话 甚至可以下个宝塔 在宝塔里面一键安装
最近发现我之前的域名已经给人注册绑定ip了 就离谱

### 一些技术点
1. 采用了基于多因素加权的水泵控制算法
温度：温度对植物生长和水需求产生很大影响。在温度较高的情况下，植物蒸腾作用会加快，导致水分需求增加。因此，当温度较高时，可以适当提高水泵的挡位。
湿度：空气湿度同样会影响植物的水分需求。在湿度较低的情况下，植物蒸腾作用加快，需要更多的水分。因此，当湿度较低时，可以适当提高水泵的挡位。
光照强度：光照强度会影响植物的光合作用速度，从而影响水分需求。在光照强度较高的情况下，植物光合作用更加旺盛，需要更多的水分。因此，当光照强度较高时，可以适当提高水泵的挡位。
土壤湿度：土壤湿度是决定水泵开关和挡位的关键参数。当土壤湿度低于植物所需的最低水分需求时，水泵需要开启并提高挡位。当土壤湿度达到植物所需水分的最佳范围时，可以将水泵调整到适中挡位。如果土壤湿度已经很高，可以关闭水泵，以防止过度灌溉。

步骤：
1. 数据归一化
2. 计算权重
3. 计算加权和

code:
```c
void control_servo_task(void){
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

    event_flag = EVENT_NULL;
    CurrentState = Idel;
}
```
2. 通信协议的使用
i2c uart gpio 都比较简单
2. 写了一个简单的状态机 还没完善
相比实时操作系统更加简洁 写好也不见得
![](/RES/MS.png)
3. pyqt5和sql语句的使用
4. 更加理解lora和freertos的使用和原理
5. 更加了解osi中应用层协议的使用和原理

### 效果
![下位机](/RES/XWJ.jpg)
![下位机1](/RES/MO.png)
![下位机2](/RES/db.png)




