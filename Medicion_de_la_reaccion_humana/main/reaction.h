#pragma once

#include "app_events.h"

typedef enum {
    STATE_IDLE = 0,
    STATE_WAITING_LED,
    STATE_REACTING,
    STATE_MOVING,
} reaction_state_t;

void reaction_init(void);
void reaction_post_command(app_event_t type);