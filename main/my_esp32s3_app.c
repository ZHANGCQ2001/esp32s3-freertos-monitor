#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

typedef enum {
    STATE_IDLE = 0,
    STATE_PRESS_DEBOUNCE,
    STATE_PRESSED,
    STATE_RELEASE_DEBOUNCE
} ButtonState;

static const char *TAG = "board_led";

// DNESP32S3 onboard red LED: GPIO1, active low.
static const gpio_num_t BOARD_LED_GPIO = GPIO_NUM_1;
static const gpio_num_t BOARD_BTN_GPIO = GPIO_NUM_0;
static const uint32_t BOARD_LED_ON_LEVEL = 0;
static const uint32_t BOARD_LED_OFF_LEVEL = 1;
static const uint32_t DEBOUNCE_TIME = 20;


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
        .intr_type = GPIO_INTR_DISABLE,
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

void app_main(void)
{
    ESP_ERROR_CHECK(board_gpio_init());
    bool LED_STATE = false;
    int64_t current_time = 0;
    
    ButtonState buttonstate = STATE_IDLE;
    while(1) {

        int btn_state = board_btn_get();
        switch(buttonstate) {
            case(STATE_IDLE):
                if(btn_state == 0) {
                    // 检测到按键开关关闭，转入按下消抖模式，并记录下当前时间
                    current_time = esp_timer_get_time() / 1000;
                    buttonstate = STATE_PRESS_DEBOUNCE;
                }
                break;
            case(STATE_PRESS_DEBOUNCE):
                if(btn_state == 1) {
                    // 如果在检测时间内，按键回到打开状态，则返回最初状态
                    buttonstate = STATE_IDLE;
                } else if((esp_timer_get_time() / 1000 - current_time) >= DEBOUNCE_TIME) {
                    // 如果经过一段时间以后，按键开关依然保持关闭，那么跳转到按键关闭状态
                    buttonstate = STATE_PRESSED;
                }
                break;
            case(STATE_PRESSED):
                if(btn_state == 1) {
                    // 检测到按键开关打开，转入打开消抖模式，并记录下当前时间
                    current_time = esp_timer_get_time() / 1000;
                    buttonstate = STATE_RELEASE_DEBOUNCE;
                }
                break;
            case(STATE_RELEASE_DEBOUNCE):
                if(btn_state == 0) {
                    // 如果在检测时间内，按键回到关闭状态，则说明还没松手
                    buttonstate = STATE_PRESSED;
                } else if((esp_timer_get_time() / 1000 - current_time) >= DEBOUNCE_TIME) {
                    // 如果经过一段时间以后，按键开关依然保持打开，那么已经完成了一次按键按下+松手的过程
                    // 跳转到最初状态，此时LED灯状态翻转
                    buttonstate = STATE_IDLE;
                    if(LED_STATE) LED_STATE = 0;
                    else LED_STATE = 1;
                    board_led_set(LED_STATE);
                }
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
