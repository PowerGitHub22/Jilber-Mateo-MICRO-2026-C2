#pragma once

typedef void (*button_cb_t)(void);

void button_init(button_cb_t on_press);
