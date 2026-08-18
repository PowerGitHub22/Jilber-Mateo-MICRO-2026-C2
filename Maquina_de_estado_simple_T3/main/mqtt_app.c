#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_app.h"

static const char *TAG = "mqtt_app";

static esp_mqtt_client_handle_t s_client;
static mqtt_command_cb_t s_on_command;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al broker MQTT");
        esp_mqtt_client_subscribe(s_client, CONFIG_APP_MQTT_TOPIC_COMMAND, 0);
        break;

    case MQTT_EVENT_DATA: {
        int topic_len = event->topic_len;
        int data_len = event->data_len;
        if (topic_len > 63) topic_len = 63;
        if (data_len > 31) data_len = 31;

        char topic[64];
        char data[32];
        memcpy(topic, event->topic, topic_len);
        topic[topic_len] = '\0';
        memcpy(data, event->data, data_len);
        data[data_len] = '\0';

        ESP_LOGI(TAG, "Recibido [%s] = %s", topic, data);

        if (strcmp(data, "on") == 0) {
            s_on_command(EVENT_MQTT_SET_ON);
        } else if (strcmp(data, "off") == 0) {
            s_on_command(EVENT_MQTT_SET_OFF);
        } else if (strcmp(data, "toggle") == 0) {
            s_on_command(EVENT_MQTT_TOGGLE);
        } else {
            ESP_LOGW(TAG, "Comando MQTT desconocido: %s", data);
        }
        break;
    }

    case MQTT_EVENT_ERROR:
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT desconectado; se reintentara automaticamente");
        break;

    default:
        break;
    }
}

void mqtt_app_start(mqtt_command_cb_t on_command) {
    s_on_command = on_command;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_APP_MQTT_BROKER_URI,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));
}

void mqtt_app_publish_state(state_t state) {
    if (s_client == NULL) {
        return;
    }

    const char *payload = (state == STATE_LED_ON) ? "on" : "off";
    int msg_id = esp_mqtt_client_publish(s_client,
                                         CONFIG_APP_MQTT_TOPIC_STATE,
                                         payload, 0, 0, 0);
    ESP_LOGI(TAG, "Publicado [%s] = %s (msg_id=%d)",
             CONFIG_APP_MQTT_TOPIC_STATE, payload, msg_id);
}