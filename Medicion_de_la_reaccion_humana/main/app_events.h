#pragma once

#include <stdint.h>

typedef enum {
    APP_EVT_PB1_PRESS = 0,
    APP_EVT_PB1_RELEASE,
    APP_EVT_PB2_PRESS,
    APP_EVT_RESET,
} app_event_t;

typedef struct {
    app_event_t type;
    int64_t us;
} app_event_msg_t;