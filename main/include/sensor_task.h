#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    QueueHandle_t queue;
    EventGroupHandle_t event_group;
} sensor_task_context_t;

esp_err_t sensor_task_start(QueueHandle_t queue);