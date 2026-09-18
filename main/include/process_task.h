#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_data.h"

esp_err_t process_task_start(process_task_context_t* context_p);