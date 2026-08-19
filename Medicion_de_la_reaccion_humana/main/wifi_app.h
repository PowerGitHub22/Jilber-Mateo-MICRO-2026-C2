#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void wifi_app_start(void);
bool wifi_app_wait_connected(TickType_t timeout);