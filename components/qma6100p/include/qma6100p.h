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

/*
 * 对外 API
 */
esp_err_t qma6100p_init(void);

esp_err_t qma6100p_read_chip_id(uint8_t *chip_id);

#ifdef __cplusplus
}
#endif