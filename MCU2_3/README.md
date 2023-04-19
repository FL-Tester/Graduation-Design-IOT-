## 基于多因素加权的水泵控制算法
### 原理

在这个算法中，我们将温度、湿度、光照强度和土壤湿度这四个因素都考虑进去，以决定水泵的挡位。这四个因素在植物生长中都扮演着重要的角色。
通过综合这些因素，我们可以更准确地为植物提供适宜的生长环境。
土壤湿度：这是决定水泵挡位的主要因素。土壤湿度越低，意味着土壤越干燥，植物需要更多的水分。因此，我们会根据土壤湿度来调整水泵的输出。
温度：温度对植物的生长也非常重要。在较高的温度下，植物的蒸腾作用加强，可能需要更多的水分来保持生长。因此，我们将温度因素纳入考虑。
湿度：空气湿度也会影响植物对水分的需求。当空气湿度较低时，植物蒸腾作用加强，可能需要更多的水分。因此，我们同样将湿度因素考虑在内。
光照强度：光照强度影响植物的光合作用。在光照强度较高的情况下，植物的光合作用更加旺盛，对水分的需求可能会增加。
因此，我们也将光照强度因素纳入考虑。
为了综合这些因素，我们首先对每个因素进行归一化处理，将其值映射到0到1的范围内。
接下来，我们为每个因素分配权重，这些权重反映了每个因素在决定水泵挡位时的相对重要性。
在这个示例中，我们给土壤湿度分配了最高的权重，因为它与植物对水分的需求关系最为紧密。
然后，我们计算这些加权因素的总和，得到一个名为combined_factor的值。
这个值在0到1之间，代表了所有因素的综合影响。最后，我们根据这个值来设置水泵的挡位。
例如，当combined_factor的值较低时，我们会设置较高的水泵挡位，反之则设置较低的挡位。
通过这种方法，我们可以根据多个环境因素来调整水泵的挡位，从而更好地满足植物的生长需求。

在校园林木自动灌溉系统中，温度、湿度、光照强度和土壤湿度等参数都会影响水泵的开关和挡位。以下是这些参数如何影响灌溉系统的一些建议：
温度：温度对植物生长和水需求产生很大影响。在温度较高的情况下，植物蒸腾作用会加快，导致水分需求增加。因此，当温度较高时，可以适当提高水泵的挡位。
湿度：空气湿度同样会影响植物的水分需求。在湿度较低的情况下，植物蒸腾作用加快，需要更多的水分。因此，当湿度较低时，可以适当提高水泵的挡位。
光照强度：光照强度会影响植物的光合作用速度，从而影响水分需求。在光照强度较高的情况下，植物光合作用更加旺盛，需要更多的水分。因此，当光照强度较高时，可以适当提高水泵的挡位。
土壤湿度：土壤湿度是决定水泵开关和挡位的关键参数。当土壤湿度低于植物所需的最低水分需求时，水泵需要开启并提高挡位。当土壤湿度达到植物所需水分的最佳范围时，可以将水泵调整到适中挡位。如果土壤湿度已经很高，可以关闭水泵，以防止过度灌溉。
当然，具体的参数设置和植物种类、生长阶段以及气候条件等因素有关。您可以根据实际情况调整水泵的挡位。同时，为了实现更精确的控制，您可以考虑将这些参数输入到一个控制算法中，如 PID 控制器或模糊逻辑控制器，以自动调整水泵的挡位。

在这个示例中，我将温度、湿度、光照强度和土壤湿度参数都归一化到 [0, 1] 的范围。
然后，我们根据这些参数计算加权和，其中土壤湿度的权重最高（40%），其余参数的权重均为 20%。
最后，将加权和乘以 250，得到水泵的挡位值。如有需要，您可以自行调整权重值。

请注意，这是一个简化的示例，您可能需要根据实际情况调整代码。
例如，您可以调整参数的权重，以便在不同的环境条件下获得更好的灌溉效果。

### 代码大致
```c
void control_servo_task(void) {
    while (CurrentState == ControlServo) {
        if (event_flag == EVENT_READ_DATA) {
            event_flag = EVENT_NULL;
            CurrentState = ReadData;
        }
        if (event_flag == EVENT_SEND_DATA) {
            event_flag = EVENT_NULL;
            CurrentState = SendData;
        }
        // 计算归一化的土壤湿度因子
        uint16_t soil_humi_factor = (sensor_data.soil_humi - 0) / (100 - 0);
        // 计算归一化的温度和湿度因子
        uint8_t temp_min = 20;
        uint8_t temp_max = 35;
        uint8_t humi_min = 40;
        uint8_t humi_max = 80;
        // 综合各个因素并计算水泵输出
        float soil_humi_weight = 0.5;
        float temp_weight = 0.2;
        float humi_weight = 0.2;
        float light_weight = 0.1;


        // 计算归一化的光照强度因子
        uint16_t light_min = 1000;
        uint16_t light_max = 60000;
        uint16_t clamped_light = sensor_data.light;
        if (clamped_light < light_min) {
            clamped_light = light_min;
        } else if (clamped_light > light_max) {
            clamped_light = light_max;
        }
        uint16_t light_factor = (clamped_light - light_min) / (light_max - light_min);
        uint8_t clamped_temp = sensor_data.temperature;
        uint8_t clamped_humi = sensor_data.humidity;
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

        uint8_t temp_factor = (clamped_temp - temp_min) / (temp_max - temp_min);
        uint8_t humi_factor = (clamped_humi - humi_min) / (humi_max - humi_min);

        
        uint16_t combined_factor = soil_humi_weight * soil_humi_factor +
                                temp_weight * temp_factor +
                                humi_weight * humi_factor +
                                light_weight * light_factor;
        // 限幅
        if (sensor_data.water_pump < 150) {
            sensor_data.water_pump = 150;
        } else if (sensor_data.water_pump > 250) {
            sensor_data.water_pump = 250;
        }

        // 根据 combined_factor 计算水泵输出
        if (combined_factor < 0.2) {
            sensor_data.water_pump = four;
        } else if (combined_factor < 0.4) {
            sensor_data.water_pump = three;
        } else if (combined_factor < 0.6) {
            sensor_data.water_pump = two;
        } else if (combined_factor < 0.8) {
            sensor_data.water_pump = one;
        } else {
            sensor_data.water_pump = stop;
        }

        
#if debug
        printf("2.控制舵机任务:pidout = %d\n", sensor_data.water_pump); //log
#endif
        CurrentState = EVENT_NULL;
    }
}
```

