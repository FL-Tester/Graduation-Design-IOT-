#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "esp_compiler.h"
#include "esp_netif_ip_addr.h"  
#include "driver/gpio.h"
#include "driver/rmt.h"
#include "led.h"
#include "oled.h"

static const char *TAG = "gp_mcu1";
#define EXAMPLE_CHASE_SPEED_MS (10)
#define LORA_SIZE 10
#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart_queue;
uint8_t controlbuf[4] ={0x00, 0x0c, 0x17, 0x55};

static wifi_sta_config_t wifi_sta_config = {            // sta wifi config  自定义
        .ssid = "FengLE",
        .password = "987654321",
};
esp_mqtt_client_config_t mqtt_config = {                //mqtt config  服务器地址 不能改
        .uri = "mqtt://175.178.79.144",
        .port = 1883
};
typedef struct {                //mqtt publish 数据结构体
    char *topic;                //主题
    uint16_t light;             //光照强度
    uint16_t soil_humi;         //土壤湿度
    uint8_t  temperature;       //温度
    uint8_t  humidity;          //湿度
    uint8_t  water_pump;        //水泵挡位
    char *device;               //设备
} publish_data_t; 

esp_mqtt_client_handle_t client_handle;                         //mqtt句柄
void lora_init(void);
void lora_task(void *pvParameter);
void wifi_mqtt_init(void);
void mqtt_publish(void *pvParameter);
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void oled_init(void);
void led_init(void);
TaskHandle_t mqtt_publish_task_handle = NULL;
QueueHandle_t xQueueLora = NULL;

#define RMT_TX_CHANNEL RMT_CHANNEL_0
led_strip_t *strip = NULL;
// Function to set the single LED to the given RGB color
void set_rgb(uint32_t r, uint32_t g, uint32_t b);
void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_err_t err = nvs_flash_init(); 
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);                                           //初始化nvs_flash    
    gpio_config_t pGPIOConfig = 
    {
        .intr_type      = GPIO_INTR_DISABLE,
        .mode           = GPIO_MODE_OUTPUT,
        .pin_bit_mask   = GPIO_SEL_46,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .pull_up_en     = GPIO_PULLUP_ENABLE
    };
    gpio_config(&pGPIOConfig);
    gpio_set_level(46, 1);
    led_init();
    oled_init();
    lora_init();
    xQueueLora = xQueueCreate(10, sizeof(publish_data_t));     //创建消息队列
    xTaskCreatePinnedToCore (lora_task, "lora_task", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore (mqtt_publish, "mqtt_publish", 4096, NULL, 1, &mqtt_publish_task_handle, 1);
    wifi_mqtt_init();                                               //初始化wifi和mqtt
    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
void oled_init(void){
    i2c_master_init();
    ESP_LOGI(TAG, "I2C initialized successfully");
    // OLED屏幕初始化
    OLED_Init();
    OLED_ShowCHinese(2 * 18, 0, 0);
    OLED_ShowCHinese(4 * 18, 0, 1);
    //第二页 全是横线
    OLED_ShowCHinese(0, 2, 10);
    OLED_ShowCHinese(18, 2, 10);
    OLED_ShowCHinese(18 * 2, 2, 10);
    OLED_ShowCHinese(18 * 3, 2, 10);
    OLED_ShowCHinese(18 * 4, 2, 10);
    OLED_ShowCHinese(18 * 5, 2, 10);
    OLED_ShowCHinese(18 * 6, 2, 10);
    //显示节点1:√
    OLED_ShowCHinese(0, 3, 2);
    OLED_ShowCHinese(18, 3, 3);
    OLED_ShowChar (2 * 18, 3, '1', 16);
    OLED_ShowChar (3 * 18 , 3, ':', 16);
    OLED_ShowCHinese(18 * 4, 3, 5);
    OLED_ShowCHinese(18 * 5, 3, 9);
    //OLED_ShowNum(18 * 6, 3, 0, 1, 16);
    //显示节点2:√
    OLED_ShowCHinese(0, 6, 2);
    OLED_ShowCHinese(18, 6, 3);
    OLED_ShowChar (2 * 18, 6, '2', 16);
    OLED_ShowChar (3 * 18 , 6, ':', 16);
    OLED_ShowCHinese(18 * 4, 6, 5);
    OLED_ShowCHinese(18 * 5, 6, 9);
    //OLED_ShowNum(18 * 6, 6, 0, 1, 16);

}
void mqtt_publish(void *pvParameter){
    publish_data_t publish_data_mqtt;
    //等待任务通知
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while(1){
        //等待消息队列
        xQueueReceive(xQueueLora, &publish_data_mqtt, portMAX_DELAY);
        //创建JSON数据
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "temperature", publish_data_mqtt.temperature);
        cJSON_AddNumberToObject(root, "humidity", publish_data_mqtt.humidity);
        cJSON_AddNumberToObject(root, "light", publish_data_mqtt.light);
        cJSON_AddNumberToObject(root, "soil_humi", publish_data_mqtt.soil_humi);
        cJSON_AddNumberToObject(root, "water_pump", publish_data_mqtt.water_pump);
        cJSON_AddStringToObject(root, "device", publish_data_mqtt.device);
        char *pubjson = cJSON_Print(root);
        esp_mqtt_client_publish(client_handle, publish_data_mqtt.topic, pubjson, 0, 1, 0);
        cJSON_Delete(root);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
void lora_task(void *pvParameter){  
    publish_data_t publish_data_lora ={
            .topic = "/CD/MCU1/ESP32",
            .light = 0,
            .soil_humi = 0,
            .temperature = 0,
            .humidity = 0,
            .water_pump = 0,
            .device = "nodex",
    };
    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(10);
    while(1) {
        if(xQueueReceive(uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) { 
            bzero(dtmp, 100);
            switch(event.type) {
                case UART_DATA: 
                    uart_read_bytes(UART_NUM_1, dtmp, 9, portMAX_DELAY);
                    //02 1A 48 0F 8C 00 05 0E   02代表节点号，1A温度 47湿度  0F 8C代表土壤湿度 00 05代表光照强度
                    if ((dtmp[0] == 0x02 || dtmp[0] == 0x03)){
                        publish_data_lora .temperature = dtmp[1];
                        publish_data_lora .humidity    = dtmp[2];
                        publish_data_lora .soil_humi   = dtmp[3] *256 + dtmp[4];
                        publish_data_lora .light       = dtmp[5] *256 + dtmp[6];
                        publish_data_lora .water_pump  = dtmp[7];
                        if (dtmp[0] == 0x02){
                            set_rgb(255, 0, 0);
                            publish_data_lora .device = "node1";
                            OLED_ShowCHinese(18 * 4, 3, 4);
                            OLED_ShowNum(18 * 6, 3, publish_data_lora .water_pump, 1, 16);
                            vTaskDelay(10 / portTICK_PERIOD_MS);
                            set_rgb(0, 0, 0);
                        }else if (dtmp[0] == 0x03){
                            set_rgb(0, 255, 0);
                            publish_data_lora .device = "node2";
                            OLED_ShowCHinese(18 * 4, 6, 4);
                            OLED_ShowNum(18 * 6, 6, publish_data_lora .water_pump, 1, 16);
                            vTaskDelay(10 / portTICK_PERIOD_MS);
                            set_rgb(0, 0, 0);
                        }
                        printf("节点%d:温度%d,湿度:%d土壤湿度:%d,光照强度:%d,水泵挡位:%d\n ",dtmp[0],
                        publish_data_lora .temperature,   publish_data_lora .humidity,  publish_data_lora .soil_humi,
                        publish_data_lora .light, publish_data_lora .water_pump);
                    }
                    //xTaskNotify(mqtt_publish_task_handle, 0x01, eNoAction);
                    xQueueSend(xQueueLora, &publish_data_lora , 0);
                    break;
                default:
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}
void lora_init(void){           //lora初始化
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 2 * BUF_SIZE, 20, &uart_queue, 0); 
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(UART_NUM_1, '+', 4, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM_1, 20);
}
void wifi_mqtt_init(void){
    //wifi init
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *) &wifi_sta_config);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);
    esp_wifi_start();
    //mqtt init
    client_handle = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_start(client_handle);
    esp_mqtt_client_register_event(client_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    set_rgb(0, 0, 0);
}
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
    
    if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("TEST_ESP32", "Got IP: " IPSTR,  IP2STR(&event->ip_info.ip));
        OLED_ShowCHinese(6 * 18, 0, 8);
    }
    
}
//mqtt 事件处理
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            esp_mqtt_client_publish(client_handle, "/cd/esp32/test", "mqtt connected!", 0, 0, 0);
            esp_mqtt_client_subscribe(client_handle, "/CD/SWJ", 0);
            //通知mqtt任务
            xTaskNotify(mqtt_publish_task_handle, 0x01, eNoAction);
            break;
        //数据
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            cJSON *json = cJSON_Parse(event->data);
            uint8_t control_byte = 0x00;
            if ( (cJSON_GetObjectItem(json, "node")->valueint) == 1) {
                control_byte |= (0x01 << 6);
                controlbuf[1] = 0x0c;
            } else if ( (cJSON_GetObjectItem(json, "node")->valueint) == 2) {
                control_byte |= (0x02 << 6);
                controlbuf[1] = 0x0d;
            }
            if (cJSON_GetObjectItem(json, "mode")->valueint == 1) {
                control_byte |= (0x01 << 4);
            } else if (cJSON_GetObjectItem(json, "mode")->valueint == 2) {
                control_byte |= (0x02 << 4);
            }
            if (cJSON_GetObjectItem(json, "switch")->valueint == 1) {
                control_byte |= 0x01;
            } else if (cJSON_GetObjectItem(json, "switch")->valueint == 2) {
                control_byte |= 0x02;
            }
            // Print control byte
            printf("control_byte = %x\n", control_byte);
            controlbuf[3] = control_byte;
            uart_write_bytes (UART_NUM_1, controlbuf, 4);
            printf("发送完成了\n");
            cJSON_Delete(json);
            break;
        default:
            break;

    }
}
void led_init(void){
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(48, RMT_TX_CHANNEL);
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(1, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    ESP_ERROR_CHECK(strip->clear(strip, 100));
}
void set_rgb(uint32_t r, uint32_t g, uint32_t b) {
    ESP_ERROR_CHECK(strip->set_pixel(strip, 0, r, g, b));
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
}



