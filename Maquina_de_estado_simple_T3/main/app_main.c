#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "state_machine.h"
#include "led.h"
#include "button.h"
#include "wifi_app.h"
#include "mqtt_app.h"

static const char *TAG = "app_main";

static state_machine_t s_sm;
static SemaphoreHandle_t s_sm_mutex;

static void apply_state(state_t new_state) {
    led_set(new_state == STATE_LED_ON);
    mqtt_app_publish_state(new_state);
}

static void handle_state_event(state_event_t event) {
    state_t old_state;
    state_t new_state;

    xSemaphoreTake(s_sm_mutex, portMAX_DELAY);
    old_state = s_sm.current;
    new_state = state_machine_handle_event(&s_sm, event);
    xSemaphoreGive(s_sm_mutex);

    if (new_state != old_state) {
        apply_state(new_state);
    }
}

static void on_button_press(void) {
    handle_state_event(EVENT_BUTTON_TOGGLE);
}

static void on_mqtt_command(state_event_t event) {
    handle_state_event(event);
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    s_sm_mutex = xSemaphoreCreateMutex();
    state_machine_init(&s_sm);

    led_init();
    button_init(on_button_press);

    ESP_LOGI(TAG, "Inicializado. Estado inicial: %s",
             s_sm.current == STATE_LED_ON ? "LED_ON" : "LED_OFF");

    wifi_app_start();
    wifi_app_wait_connected(portMAX_DELAY);

    mqtt_app_start(on_mqtt_command);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}