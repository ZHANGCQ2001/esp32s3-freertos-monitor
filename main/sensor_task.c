/*私有库*/
#include "sensor_task.h"
#include "sensor_data.h"
#include "qma6100p.h"
#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "system_events.h"
/*标准库*/


static const char *TAG = "sensor_task";
static void sensor_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    sensor_task_context_t *context_p = (sensor_task_context_t *) arg;
    QueueHandle_t queue = context_p->queue;
    EventGroupHandle_t event_group = context_p->event_group;
    
    const TickType_t sample_period = pdMS_TO_TICKS(100);
    TickType_t last_wake_time = xTaskGetTickCount();

    uint32_t sequence = 0;

    while (1) {

        //暂停控制
        xEventGroupWaitBits(
            event_group,
            SYS_RUN_BIT,
            pdFALSE,
            pdTRUE,
            portMAX_DELAY
        );

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

        vTaskDelayUntil(
            &last_wake_time,
            sample_period
        );
    }
}

esp_err_t sensor_task_start(sensor_task_context_t *context_p)
{
    BaseType_t ret = xTaskCreate( // 任务函数、任务名、栈大小、传递参数、优先级、Task Handle 输出
        sensor_task,
        "sensor_task",
        4096,
        context_p,
        5,
        NULL
    );

    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}