#include "udp_task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sensor_data.h"
#include "esp_cpu.h"


static const char *TAG = "udp_task";

static void udp_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t*)arg;
    uint32_t processed_count  = 0;
    while(1) {
        processed_sample_t processed_sample;
        if(xQueueReceive(
            queue,
            &processed_sample, 
            portMAX_DELAY
        ) == pdTRUE) {
            processed_count ++;
            ESP_LOGI(
                TAG,
                "X=%.3f Y=%.3f Z=%.3f |a|=%.3f g seq=%" PRIu32 " ts=%" PRId64 " us",
                processed_sample.sample.accel.x_g,
                processed_sample.sample.accel.y_g,
                processed_sample.sample.accel.z_g,
                processed_sample.norm_g,
                processed_sample.sample.sequence,
                processed_sample.sample.timestamp_us
            );
        }
    }
}

esp_err_t udp_task_start(QueueHandle_t queue)
{
    BaseType_t ret = xTaskCreate(
        udp_task,
        "udp_task",
        4096,
        queue,
        3,
        NULL
    );

    if(ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

