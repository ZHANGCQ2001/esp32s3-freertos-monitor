#pragma once

#include "esp_err.h"
#include "freertos/queue.h"

esp_err_t sensor_task_start(QueueHandle_t queue);