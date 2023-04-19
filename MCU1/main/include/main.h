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


static const char *TAG = "gp_mcu1";
#define LORA_SIZE 10
#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart_queue;

ip4_addr_t dns_server = {
    .addr = IPADDR4_INIT_BYTES(192, 168, 1, 1)
};

static wifi_sta_config_t wifi_sta_config = {            // sta wifi config  自定义
        .ssid = "FengLe",
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
TaskHandle_t mqtt_publish_task_handle = NULL;
QueueHandle_t xQueueLora = NULL;
