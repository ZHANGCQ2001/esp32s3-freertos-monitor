#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "system_events.h"

typedef struct {
    QueueHandle_t queue;
    EventGroupHandle_t event_group;
} sensor_task_context_t;

esp_err_t sensor_task_start(sensor_task_context_t *context_p);