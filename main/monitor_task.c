#include "monitor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_task_wdt.h"

#include <inttypes.h>

static const char *TAG = "monitor_task";
static TaskHandle_t monitor_task_handle = NULL;
static TimerHandle_t monitor_timer = NULL;

static void monitor_task(void *arg);
static void monitor_timer_callback(TimerHandle_t timer);


static void monitor_task(void *arg)
{
    system_stats_context_t *stats_context_p = (system_stats_context_t *)arg;

    while (1) {
        ulTaskNotifyTake(
            pdTRUE,
            portMAX_DELAY
        );

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

        ESP_ERROR_CHECK(esp_task_wdt_reset());
    }
}

static void monitor_timer_callback(TimerHandle_t timer)
{
    (void)timer;

    xTaskNotifyGive(monitor_task_handle);
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
        &monitor_task_handle
    );

    if(ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    monitor_timer = xTimerCreate(
        "monitor_timer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        NULL,
        monitor_timer_callback
    );

    if (monitor_timer == NULL) {
        vTaskDelete(monitor_task_handle);
        monitor_task_handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    if (xTimerStart(monitor_timer, 0) != pdPASS) {
        xTimerDelete(monitor_timer, 0);
        monitor_timer = NULL;

        vTaskDelete(monitor_task_handle);
        monitor_task_handle = NULL;

        return ESP_FAIL;
    }

    esp_err_t err = esp_task_wdt_add(monitor_task_handle);
    if (err != ESP_OK) {
        vTaskDelete(monitor_task_handle);
        monitor_task_handle = NULL;

        xTimerDelete(monitor_timer, portMAX_DELAY);
        monitor_timer = NULL;
        return err;
    }

    return ESP_OK;
}
