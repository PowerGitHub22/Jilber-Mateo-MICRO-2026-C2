#pragma once

typedef enum {
    STATE_LED_OFF = 0,
    STATE_LED_ON = 1,
} state_t;

typedef enum {
    EVENT_BUTTON_TOGGLE = 0,
    EVENT_MQTT_SET_ON,
    EVENT_MQTT_SET_OFF,
    EVENT_MQTT_TOGGLE,
} state_event_t;

typedef struct {
    state_t current;
} state_machine_t;

void state_machine_init(state_machine_t *sm);
state_t state_machine_handle_event(state_machine_t *sm, state_event_t event);
