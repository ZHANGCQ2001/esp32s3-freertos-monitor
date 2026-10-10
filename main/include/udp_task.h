#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "system_stats.h"

typedef struct {
    QueueHandle_t queue;
    system_stats_context_t *stats_context_p;
} udp_task_context_t;

esp_err_t udp_task_start(udp_task_context_t *context_p);