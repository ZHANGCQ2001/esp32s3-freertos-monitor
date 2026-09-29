#include "udp_task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "sensor_data.h"
#include "wifi_sta.h"
#include "lwip/sockets.h"
#include "udp_config.h"

#include <errno.h>
#include <string.h>
#include <inttypes.h>


static const char *TAG = "udp_task";

static void udp_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;

    // 创建socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        // 失败
        ESP_LOGE(
            TAG,
            "Failed to create UDP socket: errno=%d (%s)",
            errno,
            strerror(errno)
        );
        vTaskDelete(NULL);
    }
    ESP_LOGI(
        TAG,
        "UDP socket created, fd=%d",
        sock
    );
    // 创建目标地址信息
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(UDP_DEST_PORT),
    };
    int ret = inet_pton(
        AF_INET,
        UDP_DEST_IP,
        &dest_addr.sin_addr
    );
    if (ret != 1) {
        // 失败
        ESP_LOGE(
            TAG,
            "Failed to parse destination IP: ret=%d",
            ret
        );
        close(sock);
        vTaskDelete(NULL);
    } else {
        ESP_LOGI(
            TAG,
            "Destination IP configured, UDP destination: %s:%d",
            UDP_DEST_IP,
            UDP_DEST_PORT
        );
    }
    

    while(1) {
        processed_sample_t processed_sample;
        if(xQueueReceive(
            queue,
            &processed_sample, 
            portMAX_DELAY
        ) == pdTRUE) {
            if(!wifi_sta_is_connected()) {
                continue;
            }
            char payload[128];
            int len = snprintf(
                payload,
                sizeof(payload),
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
    // 创建任务
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

