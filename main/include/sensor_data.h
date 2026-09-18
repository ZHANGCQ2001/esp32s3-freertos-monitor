#pragma once

#include "qma6100p.h"
#include "freertos/queue.h"
#include <stdint.h>

typedef struct {
    uint32_t sequence;
    int64_t  timestamp_us;
    qma6100p_accel_g_t accel;
} sensor_sample_t;

typedef struct {
    sensor_sample_t sample;
    float norm_g;
} processed_sample_t;

typedef struct {
    QueueHandle_t input_queue;
    QueueHandle_t output_queue;
} process_task_context_t;