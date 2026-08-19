#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "app_events.h"
#include "buttons.h"
#include "app_pins.h"

#define BUTTON_DEBOUNCE_US (30 * 1000)

static QueueHandle_t s_evt_queue;
static volatile int64_t s_last_edge_us[2];

static void buttons_isr(void *arg) {
    gpio_num_t gpio = (gpio_num_t)(intptr_t)arg;
    int idx = (gpio == APP_PB1_GPIO) ? 0 : 1;
    int64_t now = esp_timer_get_time();

    if (now - s_last_edge_us[idx] < BUTTON_DEBOUNCE_US) {
        return;
    }
    s_last_edge_us[idx] = now;

    app_event_t evt;
    if (gpio == APP_PB1_GPIO) {
        evt = (gpio_get_level(gpio) == 0) ? APP_EVT_PB1_PRESS : APP_EVT_PB1_RELEASE;
    } else {
        if (gpio_get_level(gpio) != 0) {
            return;
        }
        evt = APP_EVT_PB2_PRESS;
    }

    app_event_msg_t msg = { .type = evt, .us = now };
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(s_evt_queue, &msg, &woken);
    portYIELD_FROM_ISR(woken);
}

static void config_pin(gpio_num_t gpio, gpio_int_type_t intr) {
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = intr,
    };
    gpio_config(&io);
}

void buttons_init(QueueHandle_t evt_queue) {
    s_evt_queue = evt_queue;

    config_pin(APP_PB1_GPIO, GPIO_INTR_ANYEDGE);
    config_pin(APP_PB2_GPIO, GPIO_INTR_NEGEDGE);

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1));
    gpio_isr_handler_add(APP_PB1_GPIO, buttons_isr, (void *)(intptr_t)APP_PB1_GPIO);
    gpio_isr_handler_add(APP_PB2_GPIO, buttons_isr, (void *)(intptr_t)APP_PB2_GPIO);
}