/*私有库*/
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "qma6100p.h"
#include "sensor_task.h"
#include "process_task.h"
#include "sensor_data.h"
#include "udp_task.h"
#include "wifi_sta.h"
#include "control_task.h"

/*标准库*/
#include <string.h>
#include <stdbool.h>

static const char *TAG = "app_main";

void app_main(void)
{
    
    ESP_ERROR_CHECK(qma6100p_init());

    // 创建队列，将数据从sensor_task转移到process_task
    QueueHandle_t sensor_queue = xQueueCreate(
        10,
        sizeof(sensor_sample_t)
    );
    if (sensor_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor queue");
        return;
    }

    // 创建队列，将数据从process_task转移到udp_task
    QueueHandle_t processed_queue = xQueueCreate(
        10,
        sizeof(processed_sample_t)
    );
    if (processed_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create process queue");
        return;
    }

    process_task_context_t process_context = {
        .input_queue = sensor_queue,
        .output_queue = processed_queue
    };

    // 先创建compinents，之后的任务需要
    ESP_ERROR_CHECK(wifi_sta_start());

    // 从流水线末端开始创建任务，使消费者先阻塞等待数据
    ESP_ERROR_CHECK(udp_task_start(processed_queue));
    ESP_ERROR_CHECK(process_task_start(&process_context));
    ESP_ERROR_CHECK(sensor_task_start(sensor_queue));
    ESP_ERROR_CHECK(control_task_start());
    
    
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
