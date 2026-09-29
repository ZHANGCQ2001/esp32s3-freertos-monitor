#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include <stdbool.h>

esp_err_t wifi_sta_start(void);
esp_err_t wifi_sta_wait_connected(TickType_t timeout);

// 外部接口
bool wifi_sta_is_connected(void);