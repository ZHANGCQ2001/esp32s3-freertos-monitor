#include "control_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "system_events.h"

#include <stdbool.h>
#include <string.h>

static const char *TAG = "control_task";
// DNESP32S3 onboard red LED: GPIO1, active low.
static const gpio_num_t BOARD_LED_GPIO = GPIO_NUM_1;
static const gpio_num_t BOARD_BTN_GPIO = GPIO_NUM_0;
static const gpio_num_t BOARD_UART_TX_GPIO = GPIO_NUM_43;
static const gpio_num_t BOARD_UART_RX_GPIO = GPIO_NUM_44;
static const uint32_t BOARD_LED_ON_LEVEL = 0;
static const uint32_t BOARD_LED_OFF_LEVEL = 1;
static const uint32_t DEBOUNCE_TIME = 20;

static TaskHandle_t control_task_handle = NULL;

static esp_err_t board_gpio_init(void);
static esp_err_t board_led_set(bool on);
static int board_btn_get(void);
static esp_err_t board_uart_init(uart_port_t uart_num, int baudrate);
static void control_task(void *arg);
static void button_gpio_isr_handler(void *arg);
static esp_err_t board_button_isr_init(void);


static esp_err_t board_gpio_init(void)
{
    const gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << BOARD_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&led_config), TAG, "Failed to configure LED GPIO");
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_LED_GPIO, BOARD_LED_OFF_LEVEL),
                        TAG,
                        "Failed to set initial LED level");

    const gpio_config_t btn_config = {
        .pin_bit_mask = 1ULL << BOARD_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&btn_config), TAG, "Failed to configure BTN GPIO");
    return ESP_OK;
}

static esp_err_t board_led_set(bool on)
{
    const uint32_t level = on ? BOARD_LED_ON_LEVEL : BOARD_LED_OFF_LEVEL;
    return gpio_set_level(BOARD_LED_GPIO, level);
}

static int board_btn_get(void)
{
    return gpio_get_level(BOARD_BTN_GPIO);
}

static esp_err_t board_uart_init(uart_port_t uart_num, int baudrate) {
    uart_config_t uart_config = {
        .baud_rate  = baudrate,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    ESP_RETURN_ON_ERROR(uart_param_config(uart_num, &uart_config), TAG, "Failed to configure UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(uart_num, BOARD_UART_TX_GPIO, BOARD_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "Failed to configure UART pins");
    ESP_RETURN_ON_ERROR(uart_driver_install(uart_num, 1024, 0, 0, NULL, 0), TAG, "Failed to configure UART driver install");
    return ESP_OK;
}

static void button_gpio_isr_handler(void *arg)
{
    (void)arg;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(
        control_task_handle,
        &xHigherPriorityTaskWoken
    );

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t board_button_isr_init(void)
{
    ESP_RETURN_ON_ERROR(
        gpio_install_isr_service(0),
        TAG,
        "Failed to install GPIO ISR service"
    );

    ESP_RETURN_ON_ERROR(
        gpio_isr_handler_add(
            BOARD_BTN_GPIO,
            button_gpio_isr_handler,
            NULL
        ),
        TAG,
        "Failed to add button ISR handler"
    );

    return ESP_OK;
}

static void control_task(void *arg) 
{
    uart_port_t uart_num  = UART_NUM_0;
    bool led_state = 0;

    int stable_btn_state = board_btn_get();
    EventGroupHandle_t event_group = (EventGroupHandle_t)arg;
    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME));
        int new_btn_state = board_btn_get();

        if(new_btn_state == stable_btn_state) {
            continue;
        }
        stable_btn_state = new_btn_state;

        if (stable_btn_state == 0) {
            // 确认按下
        } else {
            // 确认释放
            led_state = !led_state;
            ESP_ERROR_CHECK(board_led_set(led_state));

            const char *message =
                led_state ? "LED ON\r\n" : "LED OFF\r\n";

            size_t message_len = strlen(message);
            int written = uart_write_bytes(
                uart_num,
                message,
                message_len
            );
            if (written < 0) {
                ESP_LOGE(TAG, "Failed to send UART data");
            } else if ((size_t)written != message_len) {
                ESP_LOGW(TAG, "Only sent %d bytes", written);
            }

            EventBits_t bits = xEventGroupGetBits(event_group);
            if (bits & SYS_RUN_BIT) {
                // 当前 RUN
                // 清掉 SYS_RUN_BIT → PAUSE
                xEventGroupClearBits(
                    event_group,
                    SYS_RUN_BIT
                );

                ESP_LOGI(TAG, "System state: PAUSE");
            } else {
                // 当前 PAUSE
                // 设置 SYS_RUN_BIT → RUN
                xEventGroupSetBits(
                    event_group,
                    SYS_RUN_BIT
                );
                ESP_LOGI(TAG, "System state: RUN");
            }
        }
    }
}

esp_err_t control_task_start(EventGroupHandle_t event_group) 
{
    ESP_RETURN_ON_ERROR(
        board_gpio_init(),
        TAG,
        "Failed to initialize GPIO"
    );

    ESP_RETURN_ON_ERROR(
        board_uart_init(UART_NUM_0, 115200),
        TAG,
        "Failed to initialize UART"
    );
    // 创建任务
    BaseType_t ret = xTaskCreate(
        control_task,
        "control_task",
        4096,
        event_group,
        4,
        &control_task_handle
    );

    if(ret != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(
        board_button_isr_init(),
        TAG,
        "Failed to initialize button ISR"
    );

    return ESP_OK;
}
