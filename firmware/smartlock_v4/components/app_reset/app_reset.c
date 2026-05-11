#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <iot_button.h>
#include <button_gpio.h>
#include <esp_rmaker_utils.h>

static const char *TAG = "app_reset";

#define REBOOT_DELAY        2
#define RESET_DELAY         2

static void wifi_reset_trigger(void *arg, void *data)
{
    esp_rmaker_wifi_reset(RESET_DELAY, REBOOT_DELAY);
}

static void wifi_reset_indicate(void *arg, void *data)
{
    ESP_LOGI(TAG, "Release button now for Wi-Fi reset. Keep pressed for factory reset.");
}

static void factory_reset_trigger(void *arg, void *data)
{
    esp_rmaker_factory_reset(RESET_DELAY, REBOOT_DELAY);
}

static void factory_reset_indicate(void *arg, void *data)
{
    ESP_LOGI(TAG, "Release button to trigger factory reset.");
}

esp_err_t app_reset_button_register(button_handle_t btn_handle, uint8_t wifi_reset_timeout,
        uint8_t factory_reset_timeout)
{
    if (!btn_handle) return ESP_ERR_INVALID_ARG;
    esp_err_t ret = ESP_OK;

    if (wifi_reset_timeout) {
        button_event_args_t wifi_reset_args = {
            .long_press.press_time = wifi_reset_timeout * 1000,
        };
        ret |= iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_UP, &wifi_reset_args, wifi_reset_trigger, NULL);

        button_event_args_t wifi_indicate_args = {
            .long_press.press_time = wifi_reset_timeout * 1000,
        };
        ret |= iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_START, &wifi_indicate_args, wifi_reset_indicate, NULL);
    }

    if (factory_reset_timeout) {
        button_event_args_t factory_reset_args = {
            .long_press.press_time = factory_reset_timeout * 1000,
        };
        ret |= iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_UP, &factory_reset_args, factory_reset_trigger, NULL);

        button_event_args_t factory_indicate_args = {
            .long_press.press_time = factory_reset_timeout * 1000,
        };
        ret |= iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_START, &factory_indicate_args, factory_reset_indicate, NULL);
    }

    return ret;
}

button_handle_t app_reset_button_create(gpio_num_t gpio_num, uint8_t active_level)
{
    button_config_t btn_cfg = {
        .long_press_time = 0,
        .short_press_time = 0,
    };
    button_gpio_config_t gpio_cfg = {
        .gpio_num = gpio_num,
        .active_level = active_level,
        .enable_power_save = false,
    };
    button_handle_t btn_handle = NULL;
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_handle) != ESP_OK) return NULL;
    return btn_handle;
}
