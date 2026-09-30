#include "wifi_sta.h"
#include "wifi_credentials.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "wifi_sta";
static int retry_count = 0;
static EventGroupHandle_t wifi_event_group;
static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
);


static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "Wi-Fi STA started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        retry_count++;
        ESP_LOGW(
            TAG,
            "WiFi disconnected, reason=%d, retry_count=%d",
            event->reason,
            retry_count
        );

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        esp_wifi_connect();

    } else if (event_base == IP_EVENT &&
               event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(
            TAG,
            "Got IP: " IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
        retry_count = 0;
    }
}


// 外部接口
esp_err_t wifi_sta_start(void)
{
    wifi_event_group = xEventGroupCreate();
    retry_count = 0;
    if (wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_RETURN_ON_ERROR(
            nvs_flash_erase(),
            TAG,
            "Failed to erase NVS"
        );

        ret = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(
        ret,
        TAG,
        "Failed to initialize NVS"
    );

    // 网络系统
    ESP_RETURN_ON_ERROR(
        esp_netif_init(),
        TAG,
        "Failed to initialize esp_netif"
    );

    // 默认循环事件
    ESP_RETURN_ON_ERROR(
        esp_event_loop_create_default(),
        TAG,
        "Failed to create event loop"
    );

    // 创建 STA 网络接口：
    esp_netif_t *sta_netif =
        esp_netif_create_default_wifi_sta();

    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default Wi-Fi STA");
        return ESP_FAIL;
    }

    // 初始化Wifi驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(
        esp_wifi_init(&cfg),
        TAG,
        "Failed to initialize Wi-Fi"
    );

    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;

    // 注册Wifi事件
    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi_event_handler,
            NULL,
            &wifi_event_instance
        ),
        TAG,
        "Failed to register Wi-Fi event handler"
    );

    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi_event_handler,
            NULL,
            &ip_event_instance
        ),
        TAG,
        "Failed to register IP event handler"
    );

    // 设置Wifi名和密码，用于登录
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_STA_SSID,
            .password = WIFI_STA_PASSWORD,
        },
    };

    ESP_RETURN_ON_ERROR(
        esp_wifi_set_mode(WIFI_MODE_STA),
        TAG,
        "Failed to set Wi-Fi mode"
    );

    ESP_RETURN_ON_ERROR(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        ),
        TAG,
        "Failed to set Wi-Fi config"
    );

    ESP_RETURN_ON_ERROR(
        esp_wifi_start(),
        TAG,
        "Failed to start Wi-Fi"
    );
    return ESP_OK;
}


esp_err_t wifi_sta_wait_connected(TickType_t timeout)
{
    if (wifi_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,                         // 等哪个 Event Group
        WIFI_CONNECTED_BIT,     // 等哪些状态
        pdFALSE,                                  // 返回后不要清 bit
        pdFALSE,                                  // 不要求全部满足，任意一个即可
        timeout                                   // 最多等多久
    );
    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

bool wifi_sta_is_connected(void)
{
    if (wifi_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(wifi_event_group);

    return (bits & WIFI_CONNECTED_BIT) != 0;
}