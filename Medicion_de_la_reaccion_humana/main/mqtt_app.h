#pragma once

#include <stdint.h>

typedef void (*mqtt_command_cb_t)(const char *command);

void mqtt_app_start(mqtt_command_cb_t on_command);
void mqtt_app_publish_event(const char *event);
void mqtt_app_publish_result(int64_t reaction_ms, int64_t movement_ms, int64_t total_ms);