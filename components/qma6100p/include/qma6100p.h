#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QMA6100P_I2C_ADDRESS      0x12
#define QMA6100P_CHIP_ID_REG      0x00
#define QMA6100P_CHIP_ID_EXPECTED 0x90

esp_err_t qma6100p_init(void);
esp_err_t qma6100p_read_chip_id(uint8_t *chip_id);

#ifdef __cplusplus
}
#endif