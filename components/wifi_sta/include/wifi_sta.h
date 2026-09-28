#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT    BIT1
#define WIFI_SOMETHING_BIT BIT2

esp_err_t wifi_sta_start(void);
esp_err_t wifi_sta_wait_connected(TickType_t timeout);
esp_err_t wifi_sta_wait_connected(portMAX_DELAY);