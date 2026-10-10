/*私有库*/
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "process_task.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "sensor_data.h"
#include "esp_cpu.h"

/*标准库*/
#include <math.h>
#include <inttypes.h>

static const char *TAG = "process_task";

static void process_task(void *arg)
{
    process_task_context_t* context_p = (process_task_context_t*)arg;
    QueueHandle_t input_queue = context_p->input_queue;
    QueueHandle_t output_queue = context_p->output_queue;
    system_stats_context_t *stats_context_p = context_p->stats_context_p;
    while(1) {
        sensor_sample_t sample;
        processed_sample_t processed_sample;
        if(xQueueReceive(
            input_queue,
            &sample, 
            portMAX_DELAY
        ) == pdTRUE) {
            float norm = sqrtf(
                sample.accel.x_g * sample.accel.x_g +
                sample.accel.y_g * sample.accel.y_g +
                sample.accel.z_g * sample.accel.z_g
            );
            processed_sample.norm_g = norm;
            processed_sample.sample = sample;
            bool dropped = false;
            if (xQueueSend(output_queue, &processed_sample, 0) != pdTRUE) {
                dropped = true;
            }

            xSemaphoreTake(stats_context_p->mutex, portMAX_DELAY);
            stats_context_p->stats.processed_samples++;
            if (dropped) {
                stats_context_p->stats.process_dropped++;
            }
            xSemaphoreGive(stats_context_p->mutex);
        }
    }
}


esp_err_t process_task_start(process_task_context_t* context_p)
{
    BaseType_t ret = xTaskCreate( // 任务函数、任务名、栈大小、传递参数、优先级、Task Handle 输出
        process_task,
        "process_task",
        4096,
        context_p,
        4,
        NULL
    );

    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}