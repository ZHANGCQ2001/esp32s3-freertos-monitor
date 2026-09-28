#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t wifi_sta_start(void);
esp_err_t wifi_sta_wait_connected(TickType_t timeout);