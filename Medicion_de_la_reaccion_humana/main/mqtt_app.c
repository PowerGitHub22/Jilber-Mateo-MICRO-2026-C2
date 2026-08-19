#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
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
        int data_len = event->data_len;
        if (data_len > 31) {
            data_len = 31;
        }
        char data[32];
        memcpy(data, event->data, data_len);
        data[data_len] = '\0';

        ESP_LOGI(TAG, "Comando recibido = %s", data);
        if (s_on_command != NULL) {
            s_on_command(data);
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

void mqtt_app_publish_event(const char *event) {
    if (s_client == NULL) {
        return;
    }
    esp_mqtt_client_publish(s_client, CONFIG_APP_MQTT_TOPIC_STATE, event, 0, 0, 0);
    ESP_LOGI(TAG, "Evento publicado [%s] = %s", CONFIG_APP_MQTT_TOPIC_STATE, event);
}

void mqtt_app_publish_result(int64_t reaction_ms, int64_t movement_ms, int64_t total_ms) {
    if (s_client == NULL) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "t_reaccion_ms", (double)reaction_ms);
    cJSON_AddNumberToObject(root, "t_movimiento_ms", (double)movement_ms);
    cJSON_AddNumberToObject(root, "t_total_ms", (double)total_ms);
    char *payload = cJSON_PrintUnformatted(root);

    esp_mqtt_client_publish(s_client, CONFIG_APP_MQTT_TOPIC_RESULT, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Resultado publicado [%s] = %s", CONFIG_APP_MQTT_TOPIC_RESULT, payload);

    cJSON_free(payload);
    cJSON_Delete(root);
}