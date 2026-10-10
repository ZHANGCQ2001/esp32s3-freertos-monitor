#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    uint32_t udp_total;
    uint32_t udp_sent;
    uint32_t udp_wifi_drop;
    uint32_t udp_send_fail;
    uint32_t udp_format_fail;
} system_stats_t;

typedef struct {
    system_stats_t stats;
    SemaphoreHandle_t mutex;
} system_stats_context_t;