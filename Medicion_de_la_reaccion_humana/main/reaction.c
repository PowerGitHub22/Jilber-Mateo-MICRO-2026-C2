#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "app_events.h"
#include "reaction.h"
#include "buttons.h"
#include "led.h"
#include "mqtt_app.h"

static const char *TAG = "reaction";

#define EVT_QUEUE_LEN       16
#define REACTION_TASK_STACK 4096

static QueueHandle_t s_evt_queue;
static esp_timer_handle_t s_led_timer;
static reaction_state_t s_state;
static int64_t s_led_on_us;
static int64_t s_pb1_release_us;
static int64_t s_reaction_us;
static int64_t s_state_start_us;

static void led_timer_cb(void *arg) {
    s_led_on_us = esp_timer_get_time();
    led_set(true);
    s_state = STATE_REACTING;
    s_state_start_us = s_led_on_us;
    ESP_LOGI(TAG, "LED encendido: suelta el primer boton");
    mqtt_app_publish_event("suelta");
}

static void start_round(void) {
    int64_t range = CONFIG_APP_DELAY_MAX_MS - CONFIG_APP_DELAY_MIN_MS + 1;
    int64_t delay_ms = CONFIG_APP_DELAY_MIN_MS + (int64_t)(esp_random() % (uint32_t)range);

    s_led_on_us = 0;
    s_state = STATE_WAITING_LED;
    ESP_LOGI(TAG, "Ronda iniciada. LED encendera en ~%lld ms", delay_ms);
    mqtt_app_publish_event("esperando");

    esp_timer_start_once(s_led_timer, delay_ms * 1000);
}

static void invalidate(const char *reason) {
    esp_timer_stop(s_led_timer);
    led_set(false);
    s_state = STATE_IDLE;
    ESP_LOGW(TAG, "Ronda invalida: %s", reason);
    mqtt_app_publish_event("invalido");
}

static void start_moving(int64_t release_us) {
    s_pb1_release_us = release_us;
    s_reaction_us = release_us - s_led_on_us;
    s_state = STATE_MOVING;
    s_state_start_us = release_us;
    ESP_LOGI(TAG, "Tiempo de reaccion: %lld ms. Presiona el segundo boton", s_reaction_us / 1000);
    mqtt_app_publish_event("presiona_segundo");
}

static void handle_event(const app_event_msg_t *msg) {
    switch (msg->type) {
    case APP_EVT_PB1_PRESS:
        if (s_state == STATE_IDLE) {
            start_round();
        }
        break;

    case APP_EVT_PB1_RELEASE:
        if (s_state == STATE_WAITING_LED) {
            if (s_led_on_us > 0 && msg->us >= s_led_on_us) {
                start_moving(msg->us);
            } else {
                invalidate("suelto antes de la senal");
            }
        } else if (s_state == STATE_REACTING) {
            if (msg->us < s_led_on_us) {
                invalidate("suelto antes de la senal");
            } else {
                start_moving(msg->us);
            }
        }
        break;

    case APP_EVT_PB2_PRESS:
        if (s_state == STATE_MOVING) {
            int64_t movement_us = msg->us - s_pb1_release_us;
            int64_t total_us = s_reaction_us + movement_us;
            led_set(false);
            s_state = STATE_IDLE;
            ESP_LOGI(TAG, "Reaccion: %lld ms | Movimiento: %lld ms | Total: %lld ms",
                     s_reaction_us / 1000, movement_us / 1000, total_us / 1000);
            mqtt_app_publish_result(s_reaction_us / 1000, movement_us / 1000, total_us / 1000);
        }
        break;

    case APP_EVT_RESET:
        esp_timer_stop(s_led_timer);
        led_set(false);
        s_state = STATE_IDLE;
        ESP_LOGI(TAG, "Ronda cancelada por comando");
        mqtt_app_publish_event("reset");
        break;

    default:
        break;
    }
}

static void reaction_task(void *arg) {
    for (;;) {
        TickType_t wait_ticks = pdMS_TO_TICKS(100);
        int64_t now_us = esp_timer_get_time();

        if (s_state == STATE_REACTING || s_state == STATE_MOVING) {
            int64_t timeout_ms = (s_state == STATE_REACTING)
                ? CONFIG_APP_REACTION_TIMEOUT_MS
                : CONFIG_APP_MOVEMENT_TIMEOUT_MS;
            int64_t deadline_us = s_state_start_us + timeout_ms * 1000;
            if (now_us >= deadline_us) {
                invalidate("timeout");
            } else {
                int64_t remain_ms = (deadline_us - now_us) / 1000;
                wait_ticks = pdMS_TO_TICKS(remain_ms);
            }
        }

        app_event_msg_t msg;
        if (xQueueReceive(s_evt_queue, &msg, wait_ticks) == pdTRUE) {
            handle_event(&msg);
        }
    }
}

void reaction_init(void) {
    s_evt_queue = xQueueCreate(EVT_QUEUE_LEN, sizeof(app_event_msg_t));
    assert(s_evt_queue != NULL);

    esp_timer_create_args_t targs = {
        .callback = led_timer_cb,
        .name = "led_timer",
    };
    ESP_ERROR_CHECK(esp_timer_create(&targs, &s_led_timer));

    buttons_init(s_evt_queue);
    xTaskCreate(reaction_task, "reaction_task", REACTION_TASK_STACK, NULL, 4, NULL);
}

void reaction_post_command(app_event_t type) {
    if (s_evt_queue == NULL) {
        return;
    }
    app_event_msg_t msg = { .type = type, .us = esp_timer_get_time() };
    xQueueSend(s_evt_queue, &msg, 0);
}