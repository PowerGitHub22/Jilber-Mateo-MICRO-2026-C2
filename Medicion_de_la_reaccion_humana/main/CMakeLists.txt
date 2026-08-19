idf_component_register(
    SRCS
        "app_main.c"
        "reaction.c"
        "buttons.c"
        "led.c"
        "wifi_app.c"
        "mqtt_app.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_wifi esp_netif esp_event mqtt json esp_driver_gpio esp_timer esp_system
)