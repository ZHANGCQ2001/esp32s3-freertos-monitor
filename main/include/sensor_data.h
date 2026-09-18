#pragma once

#include "qma6100p.h"


typedef struct {
    uint32_t sequence;
    uint64_t timestamp_us;
    qma6100p_accel_g_t accel;
} sensor_sample_t;