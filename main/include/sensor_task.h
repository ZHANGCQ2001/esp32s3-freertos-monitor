#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "system_events.h"
#include "system_stats.h"

typedef struct {
    QueueHandle_t queue;
    EventGroupHandle_t event_group;
    system_stats_context_t *stats_context_p;
} sensor_task_context_t;

esp_err_t sensor_task_start(sensor_task_context_t *context_p);