#include "qma6100p.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"


/* ==================== I2C 配置 ==================== */

/*
 * 这些值属于“板级 / ESP-IDF 配置”
 *
 * I2C_NUM_0：
 *   正点原子例程选择 I2C0
 *
 * GPIO41 / GPIO42：
 *   DNESP32S3 板上 QMA6100P 的 SDA / SCL
 *
 * 400000：
 *   400 kHz I2C Fast Mode
 *
 * 1000 ms：
 *   当前 bring-up 阶段设置的较宽松软件超时，
 *   不是 QMA6100P Datasheet 强制要求
 */
#define QMA6100P_I2C_PORT        I2C_NUM_0
#define QMA6100P_I2C_SDA         GPIO_NUM_41
#define QMA6100P_I2C_SCL         GPIO_NUM_42
#define QMA6100P_I2C_FREQ_HZ     400000
#define QMA6100P_I2C_TIMEOUT_MS  1000


/* ==================== QMA6100P 内部定义 ==================== */

/*
 * RESET 寄存器和命令值来自 QMA6100P 芯片协议。
 *
 * 正点原子参考驱动中也定义：
 * RESET register = 0x36
 * RESET command  = 0xB6
 * RESET end      = 0x00
 */
#define QMA6100P_RESET_REG       0x36
#define QMA6100P_RESET_CMD       0xB6
#define QMA6100P_RESET_END       0x00


/* ==================== 模块内部状态 ==================== */

/*
 * static：
 * 只允许 qma6100p.c 自己使用。
 *
 * main.c 不需要知道 TAG、I2C bus handle、
 * device handle 到底是什么。
 */
static const char *TAG = "qma6100p";

static i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_master_dev_handle_t s_dev_handle = NULL;


/* ==================== 内部辅助函数 ==================== */

/*
 * 这些函数都是驱动的“实现细节”。
 *
 * main.c 不应该直接调用它们，
 * 所以全部加 static。
 */


/*
 * 从 start_reg 开始连续读取 len 个字节。
 *
 * 例如：
 * start_reg = 0x00, len = 1
 *     → 读取 CHIP_ID
 *
 * start_reg = 0x01, len = 6
 *     → 以后读取 X/Y/Z 六个字节
 */
static esp_err_t qma6100p_read_regs(
    uint8_t start_reg,
    uint8_t *data,
    size_t len)
{
    if (s_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_transmit_receive(
        s_dev_handle,

        &start_reg,
        1,      /* QMA6100P 内部寄存器地址长度 = 1 byte */

        data,
        len,    /* 实际需要读取的数据长度 */

        QMA6100P_I2C_TIMEOUT_MS
    );
}


/*
 * 向一个 QMA6100P 寄存器写入一个字节。
 *
 * buffer[0] = 寄存器地址
 * buffer[1] = 数据
 *
 * 注意：
 * 这不是 ESP-IDF 规定的格式，
 * 而是 QMA6100P 的寄存器访问协议。
 */
static esp_err_t qma6100p_write_reg(
    uint8_t reg,
    uint8_t value)
{
    if (s_dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t buffer[2] = {
        reg,
        value
    };

    return i2c_master_transmit(
        s_dev_handle,
        buffer,
        sizeof(buffer),
        QMA6100P_I2C_TIMEOUT_MS
    );
}


/*
 * QMA6100P 软件复位。
 *
 * 当前只是实现 RESET 序列的前半部分。
 * 后面我们还会加入状态寄存器检查。
 */
static esp_err_t qma6100p_soft_reset(void)
{
    esp_err_t ret;

    ret = qma6100p_write_reg(
        QMA6100P_RESET_REG,
        QMA6100P_RESET_CMD
    );

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * Datasheet 要求软件复位命令之后等待约 1 ms。
     *
     * 当前 FreeRTOS：
     * CONFIG_FREERTOS_HZ = 100
     * 1 tick = 10 ms
     *
     * 因此这里不用 vTaskDelay(1)，
     * 而使用微秒级延时。
     */
    esp_rom_delay_us(1000);

    ret = qma6100p_write_reg(
        QMA6100P_RESET_REG,
        QMA6100P_RESET_END
    );

    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}


/* ==================== 对外公开 API ==================== */


/*
 * 对外公开：
 * main.c 需要调用它。
 *
 * 当前阶段它负责：
 * 1. 创建 ESP32-S3 I2C bus
 * 2. 把 QMA6100P 注册到 bus
 *
 * 后面完成完整芯片初始化以后，
 * soft_reset() 等流程会逐渐放进这里。
 */
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
        i2c_new_master_bus(
            &bus_config,
            &s_bus_handle
        ),
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


/*
 * 对外公开：
 * main.c 目前需要它来做 bring-up 验证。
 *
 * 但它内部不再直接操作 ESP-IDF I2C API，
 * 而是复用内部 qma6100p_read_regs()。
 */
esp_err_t qma6100p_read_chip_id(uint8_t *chip_id)
{
    if (chip_id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    return qma6100p_read_regs(
        QMA6100P_CHIP_ID_REG,
        chip_id,
        1
    );
}