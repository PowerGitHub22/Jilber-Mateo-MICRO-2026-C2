idf_component_register(
    SRCS
        "app_main.c"
        "state_machine.c"
        "led.c"
        "button.c"
        "wifi_app.c"
        "mqtt_app.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_wifi esp_netif esp_event mqtt esp_driver_gpio
)
