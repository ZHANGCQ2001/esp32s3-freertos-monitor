#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_data.h"

typedef struct {
    QueueHandle_t input_queue;
    QueueHandle_t output_queue;
} process_task_context_t;

esp_err_t process_task_start(process_task_context_t* context_p);