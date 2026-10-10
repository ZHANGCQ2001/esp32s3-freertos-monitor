#include "monitor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <inttypes.h>

static const char *TAG = "monitor_task";


static void monitor_task(void *arg)
{
    system_stats_context_t *stats_context_p =
        (system_stats_context_t *)arg;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        system_stats_t snapshot;

        xSemaphoreTake(
            stats_context_p->mutex,
            portMAX_DELAY
        );

        snapshot = stats_context_p->stats;

        xSemaphoreGive(stats_context_p->mutex);

        ESP_LOGI(
            TAG,
            " sensor=%" PRIu32
            " processed=%" PRIu32
            " process_drop=%" PRIu32
            " UDP total=%" PRIu32
            " sent=%" PRIu32
            " wifi_drop=%" PRIu32
            " send_fail=%" PRIu32
            " format_fail=%" PRIu32,
            snapshot.sensor_samples,
            snapshot.processed_samples,
            snapshot.process_dropped,
            snapshot.udp_total,
            snapshot.udp_sent,
            snapshot.udp_wifi_drop,
            snapshot.udp_send_fail,
            snapshot.udp_format_fail
        );
    }
}


esp_err_t monitor_task_start(system_stats_context_t *context_p)
{
    // 创建任务
    BaseType_t ret = xTaskCreate(
        monitor_task,
        "monitor_task",
        4096,
        context_p,
        2,
        NULL
    );

    if(ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
