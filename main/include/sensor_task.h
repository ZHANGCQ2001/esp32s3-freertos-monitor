#pragma once

#include "esp_err.h"
#include "qma6100p.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef struct {
    uint32_t sequence;
    TickType_t timestamp;
    qma6100p_accel_g_t accel;
} qma6100p_frame_g_t;

esp_err_t sensor_task_start(QueueHandle_t queue);