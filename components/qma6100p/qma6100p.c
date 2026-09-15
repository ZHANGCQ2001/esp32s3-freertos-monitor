#include "qma6100p.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"

#define QMA6100P_I2C_PORT      I2C_NUM_0
#define QMA6100P_I2C_SDA       GPIO_NUM_41
#define QMA6100P_I2C_SCL       GPIO_NUM_42
#define QMA6100P_I2C_FREQ_HZ   400000
#define QMA6100P_I2C_TIMEOUT_MS 1000

static const char *TAG = "qma6100p";

static i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_master_dev_handle_t s_dev_handle = NULL;

esp_err_t qma6100p_init(void)
{
    if (s_dev_handle != NULL) {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = QMA6100P_I2C_PORT,
        .scl_io_num = QMA6100P_I2C_SCL,
        .sda_io_num = QMA6100P_I2C_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_RETURN_ON_ERROR(
        i2c_new_master_bus(&bus_config, &s_bus_handle),
        TAG,
        "Failed to create I2C master bus"
    );

    const i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = QMA6100P_I2C_ADDRESS,
        .scl_speed_hz = QMA6100P_I2C_FREQ_HZ,
    };

    ESP_RETURN_ON_ERROR(
        i2c_master_bus_add_device(
            s_bus_handle,
            &dev_config,
            &s_dev_handle
        ),
        TAG,
        "Failed to add QMA6100P device"
    );

    ESP_LOGI(
        TAG,
        "I2C ready: SDA=%d SCL=%d addr=0x%02X",
        QMA6100P_I2C_SDA,
        QMA6100P_I2C_SCL,
        QMA6100P_I2C_ADDRESS
    );

    return ESP_OK;
}

esp_err_t qma6100p_read_chip_id(uint8_t *chip_id)
{
    if (chip_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const uint8_t reg = QMA6100P_CHIP_ID_REG;

    return i2c_master_transmit_receive(
        s_dev_handle,
        &reg,
        1,
        chip_id,
        1,
        QMA6100P_I2C_TIMEOUT_MS
    );
}