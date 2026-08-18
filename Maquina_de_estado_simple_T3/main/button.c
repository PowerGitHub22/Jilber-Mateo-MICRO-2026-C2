#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "button.h"
#include "app_pins.h"

#define BUTTON_POLL_PERIOD_MS   10
#define BUTTON_DEBOUNCE_MS      50

static button_cb_t s_on_press;
static bool s_stable_level;
static bool s_confirmed_level;
static int s_stable_count;

static void button_task(void *arg) {
    s_stable_level = gpio_get_level(APP_BUTTON_GPIO);
    s_confirmed_level = s_stable_level;
    s_stable_count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_PERIOD_MS));

        bool raw = gpio_get_level(APP_BUTTON_GPIO);

        if (raw == s_stable_level) {
            s_stable_count++;
        } else {
            s_stable_level = raw;
            s_stable_count = 0;
        }

        if (s_stable_count >= BUTTON_DEBOUNCE_MS / BUTTON_POLL_PERIOD_MS) {
            if (s_stable_level != s_confirmed_level) {
                s_confirmed_level = s_stable_level;
                if (s_confirmed_level == 0) {
                    if (s_on_press != NULL) {
                        s_on_press();
                    }
                }
            }
            s_stable_count = 0;
        }
    }
}

void button_init(button_cb_t on_press) {
    s_on_press = on_press;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << APP_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
}
