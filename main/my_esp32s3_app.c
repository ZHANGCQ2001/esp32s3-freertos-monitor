/*私有库*/
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "qma6100p.h"
#include "sensor_task.h"
#include "process_task.h"
#include "sensor_data.h"
#include "udp_task.h"
#include "wifi_sta.h"
#include "control_task.h"
#include "system_events.h"
#include "system_stats.h"
#include "monitor_task.h"

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

    // 创建eventgroup，按键控制多个task启停
    EventGroupHandle_t system_event_group = xEventGroupCreate();
    if (system_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return;
    }
    xEventGroupSetBits(
        system_event_group,
        SYS_RUN_BIT
    );

    // 创建Mutex，控制多个task之间的共享状态
    SemaphoreHandle_t stats_mutex = xSemaphoreCreateMutex();
    if(stats_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create stats mutex");
        return;
    }

    static system_stats_context_t stats_context;
    stats_context.mutex = stats_mutex;

    static process_task_context_t process_context;
    process_context.input_queue = sensor_queue;
    process_context.output_queue = processed_queue;
    process_context.stats_context_p = &stats_mutex;

    static sensor_task_context_t sensor_context;
    sensor_context.queue = sensor_queue;
    sensor_context.event_group = system_event_group;
    sensor_context.stats_context_p = &stats_mutex;
    

    static udp_task_context_t udp_context;
    udp_context.queue = processed_queue;
    udp_context.stats_context_p = &stats_context;

    // 先创建compinents，之后的任务需要
    ESP_ERROR_CHECK(wifi_sta_start());
    // 从流水线末端开始创建任务，使消费者先阻塞等待数据
    ESP_ERROR_CHECK(udp_task_start(&udp_context));
    ESP_ERROR_CHECK(process_task_start(&process_context));
    ESP_ERROR_CHECK(sensor_task_start(&sensor_context));
    ESP_ERROR_CHECK(control_task_start(system_event_group));
    ESP_ERROR_CHECK(monitor_task_start(&stats_context));
}
