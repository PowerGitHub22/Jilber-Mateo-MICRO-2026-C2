#pragma once

#include "state_machine.h"

typedef void (*mqtt_command_cb_t)(state_event_t event);

void mqtt_app_start(mqtt_command_cb_t on_command);
void mqtt_app_publish_state(state_t state);