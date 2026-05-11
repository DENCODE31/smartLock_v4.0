#include <sdkconfig.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include <iot_button.h>
#include <button_gpio.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>

#include <app_reset.h>

#include "app_priv.h"
#include "app_config.h"

static const char *TAG = "app_driver";

static bool g_lock_state = DEFAULT_LOCK_STATE;

#define BUTTON_GPIO          BUTTON_INTERIOR
#define BUTTON_ACTIVE_LEVEL  0

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static void set_lock_state(bool locked)
{
    gpio_set_level(RELAY_LOCK, locked);
#ifdef HELTEC_V3
    gpio_set_level(LED_BUILTIN, !locked);
#else
    gpio_set_level(LED_G, locked);
    gpio_set_level(LED_R, !locked);
#endif
}

static void push_btn_cb(void *arg, void *data)
{
    bool new_state = !g_lock_state;
    app_driver_set_state(new_state);
    esp_rmaker_param_update_and_report(
            esp_rmaker_device_get_param_by_name(lock_device, ESP_RMAKER_DEF_POWER_NAME),
            esp_rmaker_bool(!new_state));
}

void app_driver_init(void)
{
    button_config_t btn_cfg = {
        .long_press_time = 0,
        .short_press_time = 0,
    };
    button_gpio_config_t gpio_cfg = {
        .gpio_num = BUTTON_GPIO,
        .active_level = BUTTON_ACTIVE_LEVEL,
        .enable_power_save = false,
    };
    button_handle_t btn_handle = NULL;
    if (iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_handle) == ESP_OK && btn_handle) {
        iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, NULL, push_btn_cb, NULL);
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    io_conf.pin_bit_mask = ((uint64_t)1 << RELAY_LOCK);
#ifdef HELTEC_V3
    io_conf.pin_bit_mask |= ((uint64_t)1 << LED_BUILTIN);
    gpio_config(&io_conf);
    gpio_set_level(LED_BUILTIN, 1);
#else
    io_conf.pin_bit_mask |= ((uint64_t)1 << LED_R) | ((uint64_t)1 << LED_G);
    gpio_config(&io_conf);
#endif

    gpio_config_t buzzer = {
        .pin_bit_mask = (uint64_t)1 << BUZZER_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    gpio_config(&buzzer);
    gpio_set_level(BUZZER_PIN, 1);

    set_lock_state(g_lock_state);
    ESP_LOGI(TAG, "Driver initialized, lock state: %s", g_lock_state ? "LOCKED" : "UNLOCKED");
}

int IRAM_ATTR app_driver_set_state(bool locked)
{
    if (g_lock_state != locked) {
        g_lock_state = locked;
        set_lock_state(g_lock_state);
        gpio_set_level(BUZZER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(BUZZER_PIN, 1);
    }
    return ESP_OK;
}

bool app_driver_get_state(void)
{
    return g_lock_state;
}
