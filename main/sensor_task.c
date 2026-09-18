/*私有库*/
#include "sensor_task.h"
#include "sensor_data.h"
#include "qma6100p.h"
#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_timer.h"

/*标准库*/


static const char *TAG = "sensor_task";
static void sensor_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    QueueHandle_t queue = (QueueHandle_t)arg;
    
    const TickType_t sample_period = pdMS_TO_TICKS(100);
    TickType_t last_wake_time = xTaskGetTickCount();

    uint32_t sequence = 0;

    while (1) {
        sensor_sample_t sample;
        sample.timestamp_us = esp_timer_get_time();
        esp_err_t ret = qma6100p_read_accel_g(&sample.accel);
        if (ret == ESP_OK) {
            sample.sequence = sequence;
            if (xQueueSend(queue, &sample, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Sensor queue full, sample dropped");
            }
        } else {
            ESP_LOGE(
                TAG,
                "QMA read failed: %s",
                esp_err_to_name(ret)
            );
        }

        sequence++;
        if (sequence % 20 == 0) {
            ESP_LOGI(
                TAG,
                "core=%d, stack free min=%u bytes",
                esp_cpu_get_core_id(),
                (unsigned)uxTaskGetStackHighWaterMark(NULL)
            );
        }

        vTaskDelayUntil(
            &last_wake_time,
            sample_period
        );
    }
}

esp_err_t sensor_task_start(QueueHandle_t queue)
{
    BaseType_t ret = xTaskCreate( // 任务函数、任务名、栈大小、传递参数、优先级、Task Handle 输出
        sensor_task,
        "sensor_task",
        4096,
        queue,
        5,
        NULL
    );

    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}