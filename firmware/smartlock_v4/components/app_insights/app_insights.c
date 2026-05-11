#include <esp_err.h>
#include <esp_log.h>

static const char *TAG = "app_insights";

esp_err_t app_insights_enable(void)
{
    ESP_LOGI(TAG, "ESP Insights not enabled. Enable CONFIG_ESP_INSIGHTS_ENABLED for diagnostics.");
    return ESP_OK;
}
