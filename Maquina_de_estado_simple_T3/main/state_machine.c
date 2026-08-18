#include "state_machine.h"

void state_machine_init(state_machine_t *sm) {
    sm->current = STATE_LED_OFF;
}

state_t state_machine_handle_event(state_machine_t *sm, state_event_t event) {
    state_t next = sm->current;

    switch (sm->current) {
    case STATE_LED_OFF:
        if (event == EVENT_BUTTON_TOGGLE ||
            event == EVENT_MQTT_TOGGLE ||
            event == EVENT_MQTT_SET_ON) {
            next = STATE_LED_ON;
        }
        break;

    case STATE_LED_ON:
        if (event == EVENT_BUTTON_TOGGLE ||
            event == EVENT_MQTT_TOGGLE ||
            event == EVENT_MQTT_SET_OFF) {
            next = STATE_LED_OFF;
        }
        break;

    default:
        break;
    }

    sm->current = next;
    return next;
}
