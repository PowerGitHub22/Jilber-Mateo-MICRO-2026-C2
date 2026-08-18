#include "driver/gpio.h"
#include "led.h"
#include "app_pins.h"

void led_init(void) {
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << APP_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(APP_LED_GPIO, 0);
}

void led_set(bool on) {
    gpio_set_level(APP_LED_GPIO, on ? 1 : 0);
}
