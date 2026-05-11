#pragma once
#include <stdint.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <iot_button.h>

#ifdef __cplusplus
extern "C" {
#endif

button_handle_t app_reset_button_create(gpio_num_t gpio_num, uint8_t active_level);
esp_err_t app_reset_button_register(button_handle_t btn_handle, uint8_t wifi_reset_timeout, uint8_t factory_reset_timeout);

#ifdef __cplusplus
}
#endif
