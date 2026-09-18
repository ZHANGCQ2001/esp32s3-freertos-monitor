#pragma once

#include "esp_err.h"
#include "qma6100p.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_data.h"

esp_err_t sensor_task_start(QueueHandle_t queue);