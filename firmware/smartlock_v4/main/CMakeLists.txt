idf_component_register(
    SRCS
        "main.c"
        "managers/cloud_manager.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        wifi_provisioning
        espressif__esp_rainmaker
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE HELTEC_V3)
