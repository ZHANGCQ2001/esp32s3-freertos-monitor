#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "system_stats.h"

typedef struct {
    QueueHandle_t input_queue;
    QueueHandle_t output_queue;
    system_stats_context_t *stats_context_p;
} process_task_context_t;

esp_err_t process_task_start(process_task_context_t* context_p);