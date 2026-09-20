#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

esp_err_t udp_task_start(QueueHandle_t queue);