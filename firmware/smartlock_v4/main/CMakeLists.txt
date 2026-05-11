idf_component_register(
    SRCS
        "main.c"
        "app_driver.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        esp_driver_gpio
        app_network
        app_insights
        app_reset
        espressif__button
        espressif__esp_rainmaker
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HELTEC_V3)
