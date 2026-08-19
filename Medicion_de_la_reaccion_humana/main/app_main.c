#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_events.h"
#include "reaction.h"
#include "led.h"
#include "wifi_app.h"
#include "mqtt_app.h"

static const char *TAG = "app_main";

static void on_mqtt_command(const char *command) {
    if (strcmp(command, "reset") == 0) {
        reaction_post_command(APP_EVT_RESET);
    } else {
        ESP_LOGW(TAG, "Comando MQTT desconocido: %s", command);
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    led_init();
    reaction_init();

    ESP_LOGI(TAG, "Sistema de medicion de reaccion humana iniciado");

    wifi_app_start();
    wifi_app_wait_connected(portMAX_DELAY);

    mqtt_app_start(on_mqtt_command);
    mqtt_app_publish_event("listo");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}