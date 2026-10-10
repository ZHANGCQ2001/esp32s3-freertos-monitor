#pragma once

#include "esp_err.h"
#include "system_stats.h"

esp_err_t monitor_task_start(system_stats_context_t *stats_context_p);