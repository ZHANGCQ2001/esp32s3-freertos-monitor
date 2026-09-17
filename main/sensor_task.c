/*私有库*/
#include "sensor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "qma6100p.h"

/*标准库*/
#include <math.h>


static const char *TAG = "sensor_task";

static void sensor_task(void *arg)
{
    const TickType_t sample_period = pdMS_TO_TICKS(50);
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        qma6100p_accel_g_t accel;

        esp_err_t ret = qma6100p_read_accel_g(&accel);

        if (ret == ESP_OK) {
            float norm = sqrtf(
                accel.x_g * accel.x_g +
                accel.y_g * accel.y_g +
                accel.z_g * accel.z_g
            );

            ESP_LOGI(
                TAG,
                "X=%.3f Y=%.3f Z=%.3f |a|=%.3f g",
                accel.x_g,
                accel.y_g,
                accel.z_g,
                norm
            );
        } else {
            ESP_LOGE(
                TAG,
                "QMA read failed: %s",
                esp_err_to_name(ret)
            );
        }

        vTaskDelayUntil(
            &last_wake_time,
            sample_period
        );
    }
}

esp_err_t sensor_task_start(void)
{
    BaseType_t ret = xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}