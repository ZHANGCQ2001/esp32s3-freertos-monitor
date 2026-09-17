#pragma once

#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 当前 main.c 还会用这个值验证 CHIP_ID，
 * 所以暂时作为公开定义保留。
 */
#define QMA6100P_CHIP_ID_EXPECTED 0x90

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} qma6100p_raw_accel_t;

/*
 * 对外 API
 */
esp_err_t qma6100p_init(void);

esp_err_t qma6100p_read_chip_id(uint8_t *chip_id);

esp_err_t qma6100p_read_raw(qma6100p_raw_accel_t *raw);

#ifdef __cplusplus
}
#endif