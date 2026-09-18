/*私有库*/
#include "process_task.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "qma6100p.h"
#include "sensor_task.h"
// #include "esp_cpu.h"

/*标准库*/
#include <math.h>

static const char *TAG = "process_task";

static void process_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;

    while(1) {
        qma6100p_frame_g_t accel_frame;
        if(xQueueReceive(
            queue,
            &accel_frame.accel, 
            portMAX_DELAY
        ) == pdTRUE) {
            float norm = sqrtf(
                accel_frame.accel.x_g * accel_frame.accel.x_g +
                accel_frame.accel.y_g * accel_frame.accel.y_g +
                accel_frame.accel.z_g * accel_frame.accel.z_g
            );
            ESP_LOGI(
                TAG,
                "X=%.3f Y=%.3f Z=%.3f |a|=%.3f g",
                accel_frame.accel.x_g,
                accel_frame.accel.y_g,
                accel_frame.accel.z_g,
                norm
            );
        }
    }
}


esp_err_t process_task_start(QueueHandle_t queue)
{
    BaseType_t ret = xTaskCreate( // 任务函数、任务名、栈大小、传递参数、优先级、Task Handle 输出
        process_task,
        "process_task",
        4096,
        queue,
        4,
        NULL
    );

    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}