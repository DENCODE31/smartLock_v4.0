#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "app_config.h"

#if ENABLE_RAINMAKER
#include "managers/cloud_manager.h"

static void on_remote_cmd(bool unlock)
{
    // TODO: delegar a lock_manager cuando esté implementado
    ESP_LOGI("MAIN", "Alexa/App solicita: %s", unlock ? "UNLOCK" : "LOCK");
}
#endif

static const char *TAG = "SMARTLOCK";

#ifdef HELTEC_V3
static void init_heltec(void)
{
    gpio_config_t vext = {
        .pin_bit_mask = 1ULL << VEXT_ENABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&vext);
    gpio_set_level(VEXT_ENABLE, 0);

    gpio_config_t led = {
        .pin_bit_mask = 1ULL << LED_BUILTIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led);
    gpio_set_level(LED_BUILTIN, 1);

    ESP_LOGI(TAG, "Heltec V3: Vext enabled, LED init OK");
}

static void heartbeat(void)
{
    gpio_set_level(LED_BUILTIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(LED_BUILTIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}
#else
static void heartbeat(void)
{
    gpio_set_level(LED_R, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_R, 0);
    gpio_set_level(LED_G, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_G, 0);
    gpio_set_level(LED_B, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_B, 0);
}
#endif

void app_main(void)
{
#ifdef HELTEC_V3
    init_heltec();
#else
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_R) | (1ULL << LED_G) | (1ULL << LED_B),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
#endif

    ESP_LOGI(TAG, "SmartLock 4.0 -- F1 Validation");
    ESP_LOGI(TAG, "System ready. Waiting for peripherals...");

#if ENABLE_RAINMAKER
    ESP_ERROR_CHECK(cloud_manager_init(on_remote_cmd));
#endif

    int tick = 0;
    while (1) {
        heartbeat();
        ESP_LOGI(TAG, "Heartbeat #%d -- all GPIOs OK", ++tick);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
