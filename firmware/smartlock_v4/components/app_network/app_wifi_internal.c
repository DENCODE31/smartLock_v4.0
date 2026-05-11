#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_idf_version.h>
#include <app_network.h>
#include <app_wifi_internal.h>
#include <esp_netif.h>

#include <network_provisioning/manager.h>
#ifdef CONFIG_APP_NETWORK_PROV_TRANSPORT_BLE
#include <network_provisioning/scheme_ble.h>
#else
#include <network_provisioning/scheme_softap.h>
#endif

static const char* TAG = "app_wifi";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
    }
}

static void prov_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == NETWORK_PROV_EVENT) {
        switch (event_id) {
            case NETWORK_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case NETWORK_PROV_WIFI_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials\n\tSSID: %s", (const char *) wifi_sta_cfg->ssid);
                break;
            }
            case NETWORK_PROV_WIFI_CRED_FAIL: {
                network_prov_wifi_sta_fail_reason_t *reason = (network_prov_wifi_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed! Reason: %s",
                         (*reason == NETWORK_PROV_WIFI_STA_AUTH_ERROR) ?
                         "auth error" : "AP not found");
                break;
            }
            case NETWORK_PROV_WIFI_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
                break;
            default:
                break;
        }
    }
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t app_wifi_internal_init(void)
{
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL));
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    return ESP_OK;
}

esp_err_t app_wifi_internal_start(const char *pop, const char *service_name,
                                  const char *service_key, uint8_t *mfg_data,
                                  size_t mfg_data_len, bool *provisioned)
{
    network_prov_mgr_config_t config = {
#ifdef CONFIG_APP_NETWORK_RESET_PROV_ON_FAILURE
        .network_prov_wifi_conn_cfg = {
            .wifi_conn_attempts = CONFIG_APP_NETWORK_PROV_MAX_RETRY_CNT,
        },
#endif
#ifdef CONFIG_APP_NETWORK_PROV_TRANSPORT_BLE
        .scheme = network_prov_scheme_ble,
        .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
#else
        .scheme = network_prov_scheme_softap,
        .scheme_event_handler = NETWORK_PROV_EVENT_HANDLER_NONE,
#endif
    };

    ESP_ERROR_CHECK(network_prov_mgr_init(config));
    network_prov_mgr_is_wifi_provisioned(provisioned);

    if (!(*provisioned)) {
        ESP_LOGI(TAG, "Starting provisioning");
        network_prov_security_t security = NETWORK_PROV_SECURITY_1;

#ifdef CONFIG_APP_NETWORK_PROV_TRANSPORT_BLE
        uint8_t custom_service_uuid[] = {
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        network_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        if (mfg_data) {
            network_prov_scheme_ble_set_mfg_data(mfg_data, mfg_data_len);
        }
#endif

        ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(security, pop, service_name, service_key));
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        network_prov_mgr_deinit();
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
        wifi_init_sta();
    }
    return ESP_OK;
}
