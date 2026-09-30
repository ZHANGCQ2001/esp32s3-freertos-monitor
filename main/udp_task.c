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
#include <stdio.h>


static const char *TAG = "udp_task";
typedef enum {
    UDP_SAMPLE_SENT,
    UDP_SAMPLE_WIFI_DROP,
    UDP_SAMPLE_FORMAT_FAIL,
    UDP_SAMPLE_SEND_FAIL,
} udp_sample_result_t;

static void udp_task(void *arg);
static udp_sample_result_t udp_send_sample(
    int sock,
    const struct sockaddr_in *dest_addr,
    const processed_sample_t *processed_sample
);

static void udp_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;
    uint32_t sent_count = 0;
    uint32_t send_fail_count = 0;
    uint32_t wifi_drop_count = 0;
    uint32_t format_fail_count = 0;
    uint32_t total_count = 0;

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
            udp_sample_result_t transmit_status = udp_send_sample(
                sock,
                &dest_addr,
                &processed_sample
            );
            switch(transmit_status) {
                case UDP_SAMPLE_SENT:
                    sent_count++;
                    break;
                case UDP_SAMPLE_WIFI_DROP:
                    wifi_drop_count++;
                    break;
                case UDP_SAMPLE_FORMAT_FAIL:
                    format_fail_count++;
                    break;
                case UDP_SAMPLE_SEND_FAIL:
                    send_fail_count++;
                    break;
                default:
                    break;
            }
            total_count++;
            if(total_count % 100 == 0) {
                ESP_LOGI(
                    "UDP stats",
                    "total=%" PRIu32
                    " sent=%" PRIu32
                    " wifi_drop=%" PRIu32
                    " send_fail=%" PRIu32
                    " format_fail=%" PRIu32,
                    total_count,
                    sent_count,
                    wifi_drop_count,
                    send_fail_count,
                    format_fail_count
                );
            }
        }
    }
}

static udp_sample_result_t udp_send_sample(
    int sock,
    const struct sockaddr_in *dest_addr,
    const processed_sample_t *processed_sample
)
{
    if(!wifi_sta_is_connected()) {
        return UDP_SAMPLE_WIFI_DROP;
    }
    char payload[128];
    int len = snprintf(
        payload,
        sizeof(payload),
        "X=%.3f Y=%.3f Z=%.3f |a|=%.3f g seq=%" PRIu32 " ts=%" PRId64 " us",
        processed_sample->sample.accel.x_g,
        processed_sample->sample.accel.y_g,
        processed_sample->sample.accel.z_g,
        processed_sample->norm_g,
        processed_sample->sample.sequence,
        processed_sample->sample.timestamp_us
    );
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to format UDP payload");
        return UDP_SAMPLE_FORMAT_FAIL;
    }

    if ((size_t)len >= sizeof(payload)) {
        ESP_LOGW(TAG, "UDP payload truncated");
        return UDP_SAMPLE_FORMAT_FAIL;
    }
    ssize_t sent = sendto(
        sock,
        payload,
        len,
        0,
        (struct sockaddr *)dest_addr,
        sizeof(*dest_addr)
    );
    if(sent < 0) {
        int err = errno;
        ESP_LOGW(
            TAG,
            "UDP send failed: errno=%d(%s)", 
            err,
            strerror(err)
        );
        return UDP_SAMPLE_SEND_FAIL;
    }
    if(sent != len) {
        ESP_LOGW(
            TAG,
            "UDP send length mismatch: expected=%d sent=%d",
            len,
            (int)sent
        );
        return UDP_SAMPLE_SEND_FAIL;
    }

    return UDP_SAMPLE_SENT;
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

